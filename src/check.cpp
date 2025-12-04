#include <Arduino.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "check.hpp"
#include "led.hpp"
#include "wifi.hpp"

// Global secure client (never delete — reused forever)
WiFiClientSecure *client = nullptr;

// Global HTTPClient (reused)
HTTPClient https;

// Atomic counters (persist to SPIFFS)
std::atomic<uint32_t> SuccCount(0);
std::atomic<uint32_t> TimeoutCount(0);
std::atomic<uint32_t> ErrCount(0);

// Latest round-trip time in ms
std::atomic<long> Elapsed(VALUE_MISSING);

// Recent error history bitfield
std::atomic<recenterror_t> RecentError(0);

uint8_t payload[16] = {0xFD, 0xAA, 0xAA, 0x65, 0xBE, 0x9A, 0x7A, 0xA1,
                       0x46, 0x84, 0xB9, 0xDD, 0x29, 0x64, 0x98, 0x80};

const char *serverUrl = "https://152.136.99.118:42081/Maimai2Servlet/"
                        "250b3482854e7697de7d8eb6ea1fabb1";

// ---------------------------------------------------------------------------
// Load stats from SPIFFS at boot
// ---------------------------------------------------------------------------
void loadStatsFromSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("[SPIFFS] Failed to mount");
    return;
  }

  File f = SPIFFS.open("/DX1.50", "r");
  if (!f)
  {
    Serial.println("[SPIFFS] No saved stats, starting from zero");
    return;
  }

  uint32_t buf[3];
  size_t read_bytes = f.readBytes((char *)buf, sizeof(buf));
  if (read_bytes == sizeof(buf))
  {
    SuccCount.store(buf[0]);
    TimeoutCount.store(buf[1]);
    ErrCount.store(buf[2]);
    Serial.printf("[SPIFFS] Loaded: Succ=%u Timeout=%u Err=%u\n", buf[0],
                  buf[1], buf[2]);
  }
  else
  {
    Serial.printf("Expected %zu, found %zu bytes\n", sizeof(buf), read_bytes);
  }
  f.close();
}

// ---------------------------------------------------------------------------
// Save stats to SPIFFS (called every minute)
// ---------------------------------------------------------------------------
void saveStatsToSPIFFS()
{
  File f = SPIFFS.open("/DX1.50", "w");
  if (!f)
  {
    Serial.println("[SPIFFS] Failed to open for writing");
    return;
  }
  uint32_t buf[3] = {SuccCount.load(), TimeoutCount.load(), ErrCount.load()};
  f.write((uint8_t *)buf, sizeof(buf));
  f.close();
  Serial.printf("[SPIFFS] Saved: Succ=%u Timeout=%u Err=%u\n", buf[0], buf[1],
                buf[2]);
}

// ---------------------------------------------------------------------------
// One-time setup
// ---------------------------------------------------------------------------
void maimai_check_setup()
{
  // Initialize SPIFFS and load previous counters
  loadStatsFromSPIFFS();

  // Create secure client (only once)
  client = new WiFiClientSecure;
  if (!client)
  {
    Serial.println("[HTTPS] Failed to allocate WiFiClientSecure");
    return;
  }

  client->setInsecure();
  https.setReuse(true);
  https.setTimeout(20000);
}

// ---------------------------------------------------------------------------
// Core check function — reconnects automatically on failure
// ---------------------------------------------------------------------------
long maimai_check()
{
  unsigned long startTime = esp_timer_get_time();

  // Reconnect if connection was lost
  if (!https.connected())
  {
    https.end(); // clean up old state

    Serial.println("[HTTPS] (Re)connecting to server...");
    if (!https.begin(*client, serverUrl))
    {
      Serial.println("[HTTPS] begin() failed");
      start_flash_light(100, 30); // fast red blink = unreachable
      return 0;
    }

    // These headers MUST be added after every begin()
    https.addHeader("Host", "maimai-gm.wahlap.com:42081");
    https.addHeader("Accept", "*/*");
    https.addHeader("user-agent", "250b3482854e7697de7d8eb6ea1fabb1#");
    https.addHeader("Mai-Encoding", "1.50");
    https.addHeader("Charset", "UTF-8");
    https.addHeader("content-encoding", "deflate");
    https.addHeader("expect", "100-continue");
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Content-Length", "16");
  }

  int httpCode = https.POST(payload, sizeof(payload));
  long elapsed = (esp_timer_get_time() - startTime) / 1000LL;

  recenterror_t mask;
  if (httpCode <= 0)
  {
    Serial.printf("[HTTPS] POST failed, code=%d, elapsed=%ldms\n", httpCode,
                  elapsed);
    start_flash_light(100, 30);
    mask = REQUEST_FAILED;
    ErrCount.fetch_add(1);
  }
  else if (httpCode == HTTP_CODE_OK || httpCode == 100)
  {
    SuccCount.fetch_add(1);

    if (elapsed >= 4000)
    {
      start_flash_light(2000, 3);
      mask = REQUEST_TIMEOUT_MESSY;
      TimeoutCount.fetch_add(1);
    }
    else if (elapsed >= 2000)
    {
      start_flash_light(1000, 4);
      mask = REQUEST_TIMEOUT_LONG;
      TimeoutCount.fetch_add(1);
    }
    else if (elapsed >= 1000)
    {
      start_flash_light(500, 8);
      mask = REQUEST_TIMEOUT;
      TimeoutCount.fetch_add(1);
    }
    else
    {
      mask = REQUEST_SUCCEED;
    }
  }
  else
  {
    Serial.printf("[HTTPS] Unexpected HTTP code: %d\n", httpCode);
    mask = REQUEST_FAILED;
    ErrCount.fetch_add(1);
  }

  Elapsed.store(elapsed);

  // Update rolling error bitfield
  recenterror_t recent = RecentError.load();
  recent <<= BITS_OF_STATUS;
  recent |= mask;

  RecentError.store(recent);

  return elapsed;
}

// ---------------------------------------------------------------------------
// Worker task — runs forever
// ---------------------------------------------------------------------------
void maimai_check_worker(void *pvParameters)
{
  for (;;)
  {
    if (WIFI_DISCONNECTED)
    {
      delay(100);
      continue;
    }

    long elapsed = maimai_check();
    // Adaptive delay: fast when healthy, slower when sick
    if (elapsed > 0 && elapsed < 1500)
    {
      delay(500); // healthy → check frequently
    }
    else
    {
      delay(2000); // sick or failed → give server a break
    }
  }
}

// ---------------------------------------------------------------------------
// Persistence worker — saves stats every minute
// ---------------------------------------------------------------------------
void maimai_persist_worker(void *pvParameters)
{
  for (;;)
  {
    delay(60000); // every 60 seconds
    saveStatsToSPIFFS();
  }
}

// ---------------------------------------------------------------------------
// Call this once from your main setup() after WiFi is connected
// ---------------------------------------------------------------------------
void spawn_maimai_check()
{
  maimai_check_setup(); // init client + load stats

  xTaskCreatePinnedToCore(maimai_check_worker, "maimai_check", 8192, NULL,
                          10, // high priority
                          NULL,
                          1 // run on core 1 (leave core 0 for WiFi)
  );

  xTaskCreatePinnedToCore(maimai_persist_worker, "maimai_persist", 4096, NULL,
                          1, // low priority
                          NULL, 1);
}
#include <Arduino.h>

#include "check.hpp"
#include "led.hpp"
#include "recent_error.hpp"
#include "wifi.hpp"

WiFiClientSecure *client = new WiFiClientSecure;
HTTPClient https;

std::atomic<long> Elapsed(VALUE_MISSING);
std::atomic<uint32_t> SuccCount(0), TimeoutCount(0), ErrCount(0);

void maimai_check_setup() {
  const char *serverUrl = "https://152.136.99.118:42081/Maimai2Servlet/"
                          "250b3482854e7697de7d8eb6ea1fabb1";

  if (!client) {
    Serial.println("Failed to create secure client");
    return;
  }
  client->setInsecure();
  https.setReuse(true);

  https.setTimeout((uint16_t)-1); // very large timeout
  if (!https.begin(*client, serverUrl)) {
    Serial.println("[HTTPS] Unable to connect to server");
    Serial.println("Server is down");
    delete client;
    return;
  }

  // Add all required headers
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

long maimai_check() {
  static uint8_t payload[] = {0xFD, 0xAA, 0xAA, 0x65, 0xBE, 0x9A, 0x7A, 0xA1,
                              0x46, 0x84, 0xB9, 0xDD, 0x29, 0x64, 0x98, 0x80};

  // POST with raw binary payload
  unsigned long startTime = (unsigned long)esp_timer_get_time();
  int httpCode =
      https.POST(reinterpret_cast<uint8_t *>(payload), sizeof(payload));
  long elapsed = ((unsigned long)esp_timer_get_time() - startTime) / 1000;

  if (httpCode <= 0) {
    Serial.println("Server is unrechable");
    start_flash_light(100, 30);
    return 0;
  }

  // 100 or 2xx
  if (httpCode == 100 || httpCode / 100 == 2) {
    // not setting color: single-color screen
    if (elapsed >= 4000) {
      start_flash_light(2000, 3);
    } else if (elapsed >= 2000) {
      start_flash_light(1000, 4);
    } else if (elapsed >= 1000) {
      start_flash_light(500, 8);
    }
  } else {
    // TODO: handle unexpected http code, by error code
  }

  return elapsed;
}

void maimai_check_worker(void *) {
  recenterror_t recent_error = 0;

  for (;;) {
    if (WIFI_DISCONNECTED) {
      delay(100);
      continue;
    }
    long elapsed = maimai_check();
    Elapsed.store(elapsed);

    recenterror_t mask;

    if (elapsed <= 0)
      mask = REQUEST_FAILED;
    else if (elapsed >= 4000)
      mask = REQUEST_TIMEOUT_MESSY;
    else if (elapsed >= 2000)
      mask = REQUEST_TIMEOUT_LONG;
    else if (elapsed >= 1000)
      mask = REQUEST_TIMEOUT;
    else
      mask = REQUEST_SUCCEED;

    recent_error <<= BITS_OF_STATUS;
    recent_error |= mask;
    RecentError.store(recent_error);

    switch (mask) {
    case REQUEST_FAILED:
      ErrCount.fetch_add(1);
      break;

    case REQUEST_TIMEOUT:
    case REQUEST_TIMEOUT_LONG:
    case REQUEST_TIMEOUT_MESSY:
      TimeoutCount.fetch_add(1);
    case REQUEST_SUCCEED:
      SuccCount.fetch_add(1);
    default:
      break;
    }

    if (elapsed > 0 && elapsed < 1000) {
      delay(500);
    } else {
      delay(2000);
    }
  }
}

void spawn_maimai_check() {
  xTaskCreate(maimai_check_worker, "maimai_check", 8000, NULL,
              ESP_TASK_PRIO_MAX / 2, NULL);
}

#include <atomic>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_task.h>

#define LED_PIN 2
#define LED_FREQ       5000
#define LED_RESOLUTION 8

// 50% brightness
#define LED_on ledcWrite(LED_PIN, 128);
#define LED_off ledcWrite(LED_PIN, 0);

// ==================== WiFi Credentials ====================
const char *ssid = "Mk-wifi";
const char *password = "Y6SXeuMDReX $rx@[KRm";
// ==================== Request Details ====================
// The exact binary payload from your curl (16 bytes)
uint8_t payload[] = {0xFD, 0xAA, 0xAA, 0x65, 0xBE, 0x9A, 0x7A, 0xA1,
                     0x46, 0x84, 0xB9, 0xDD, 0x29, 0x64, 0x98, 0x80};
const char *serverUrl = "https://152.136.99.118:42081/Maimai2Servlet/"
                        "250b3482854e7697de7d8eb6ea1fabb1";

std::atomic<bool> Flashing(false);
struct Arguments {
  unsigned times;
  unsigned interval;
} FlashLight;

void flash_led(void *) {
  bool led_on = false;
  for (;;) {
    if (!Flashing.load()) {
      delay(100);
      continue;
    }
    if (FlashLight.times <= 0) {
      Flashing.store(false);
      LED_off;
      led_on = false;
      continue;
    }
    FlashLight.times--;
    if (led_on) {
      led_on = false;
      LED_off;
    } else {
      led_on = true;
      LED_on;
    }
    delay(FlashLight.interval);
  }
}

void start_flash_light(unsigned interval_ms, unsigned times) {
  // ignore if already flashing
  if (Flashing.load()) {
    return;
  }

  FlashLight.interval = interval_ms;
  FlashLight.times = times;
  Flashing.store(true);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // initialize light
  ledcAttach(LED_PIN, LED_FREQ, LED_RESOLUTION);
  ledcWrite(LED_PIN, 0);
  xTaskCreate(flash_led, "flash_led", 2000, NULL, ESP_TASK_PRIO_MAX - 1, NULL);

  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    start_flash_light(250, 1);
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  makeMaimaiRequest();
}

void loop() { /* do nothing */ }

void makeMaimaiRequest() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (!client) {
    Serial.println("Failed to create secure client");
    return;
  }
  client->setInsecure();
  HTTPClient https;
  https.setTimeout(15000); // 15 seconds timeout
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

  // POST with raw binary payload
  while (1) {
    unsigned long startTime = (unsigned long)esp_timer_get_time();
    int httpCode =
        https.POST(reinterpret_cast<uint8_t *>(payload), sizeof(payload));
    double elapsed =
        (double)((unsigned long)esp_timer_get_time() - startTime) / 1000.0;

    if (httpCode <= 0) {
      Serial.println("Server is unrechable");
      start_flash_light(100, 30);
      continue;
    }

    // 100 = continue
    if (httpCode == 100) {
      Serial.printf("%lf\n", elapsed);
      if (elapsed >= 4000) {
        start_flash_light(2000, 3);
      } else if (elapsed >= 2000) {
        start_flash_light(1000, 4);
      } else if (elapsed >= 1000) {
        start_flash_light(500, 8);
      }
    } else {
      // 200: under high load?
      Serial.printf("Server returned unexpected code: %d | Time: %lf ms\n",
                    httpCode, elapsed);
      start_flash_light(200, 5);
    }
    delay(1000);
  }

  https.end();
  delete client;
}

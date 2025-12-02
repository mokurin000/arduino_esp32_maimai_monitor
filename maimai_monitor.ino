#include <atomic>
#include <esp_task.h>

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// 你使用的 OLED 尺寸（最常见的是 128x64）
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ESP32 硬件 I2C 引脚
#define SDA_PIN 22
#define SCL_PIN 23

#define OLED_ADDR 0x3C

#define LED_PIN 2
// 50% brightness
#define LED_on ledcWrite(LED_PIN, 64)
#define LED_off ledcWrite(LED_PIN, 0)

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
    led_on = !led_on;
    led_on ? LED_on : LED_off;
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

void spawn_flash_task() {
  const unsigned LED_FREQ = 5000;
  const unsigned char LED_RESOLUTION = 8;
  ledcAttach(LED_PIN, LED_FREQ, LED_RESOLUTION);

  xTaskCreate(flash_led, "flash_led", 2000, NULL, ESP_TASK_PRIO_MAX - 1, NULL);
}

// 创建 I2C 实例（ESP32 默认使用 Wire，也可以自定义）
TwoWire I2CESP32 = TwoWire(0); // I2C0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2CESP32,
                         -1); // -1 表示不使用 reset 引脚
void initialise_oled() {
  // 初始化硬件 I2C（ESP32）
  I2CESP32.begin(SDA_PIN, SCL_PIN, 400000); // 400kHz 速度

  // 初始化 OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // 初始化失败，死循环
  }

  // 清屏
  display.clearDisplay();

  // 设置文字大小和颜色
  display.setTextSize(2);              // 2倍大小
  display.setTextColor(SSD1306_WHITE); // 白字

  display.setCursor(0, 0); // x=15, y=20
  display.print("Init");
  display.dim(true);

  // 把缓冲区内容推到屏幕上
  display.display();
}

void connect_wifi() {
  const char *ssid = "Mk-wifi";
  const char *password = "Y6SXeuMDReX $rx@[KRm";
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    display.print(".");
    display.display();
    start_flash_light(250, 1);
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

WiFiClientSecure *client = new WiFiClientSecure;
HTTPClient https;

void maimai_check_setup() {
  const char *serverUrl = "https://152.136.99.118:42081/Maimai2Servlet/"
                          "250b3482854e7697de7d8eb6ea1fabb1";

  if (!client) {
    Serial.println("Failed to create secure client");
    return;
  }
  client->setInsecure();

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
    Serial.printf("%ld\n", elapsed);

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

std::atomic<long> Elapsed(0);
std::atomic<unsigned> RecentError(0);

void maimai_check_worker(void *) {
  unsigned recent_error;

  for (;;) {
    long elapsed = maimai_check();
    Elapsed.store(elapsed);

    // bitvec of status
    recent_error <<= 2;
    if (elapsed <= 0)
      recent_error |= 0x01;
    if (elapsed >= 1000)
      recent_error |= 0x11;
    RecentError.store(recent_error);

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

void setup() {
  Serial.begin(115200);

  spawn_flash_task();
  initialise_oled();
  connect_wifi();
  spawn_maimai_check();
  maimai_check_setup();
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);

  unsigned recent_errors = RecentError.load();
  for (int i = 9; i >= 0; i--) {
    unsigned is_error = recent_errors >> (2 * i);
    switch (is_error & 0x11) {
    case 0x01:
      display.print("X");
      break;
    case 0x11:
      display.print("T");
      break;
    default:
      display.print("O");
    }
  }
  display.print("\n");

  int rssi = WiFi.RSSI();
  display.printf("%6d dBm\n", rssi);

  long elapsed = Elapsed.load();
  if (elapsed > 0) {
    display.printf("%6ld ms\n", elapsed);
  } else {
    display.printf("%6s ms\n SVR DOWN\n", "--");
  }

  display.display();

  delay(100);
}

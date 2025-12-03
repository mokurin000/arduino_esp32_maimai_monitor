#include <atomic>
#include <esp_task.h>

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define NO_LED_FLASHING 0

// 你使用的 OLED 尺寸（最常见的是 128x64）
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ESP32 硬件 I2C 引脚
#define SDA_PIN 33
#define SCL_PIN 32

#define OLED_ADDR 0x3C

#define LED_PIN 2
// 12.5% brightness
#define LED_on ledcWrite(LED_PIN, 32)
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
  #if NO_LED_FLASHING
  return;
  #endif
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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2CESP32, -1); // -1 表示不使用 reset 引脚
void initialise_oled() {
  // 初始化硬件 I2C（ESP32）
  I2CESP32.begin(SDA_PIN, SCL_PIN, 400000); // 400kHz 速度

  // 初始化 OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // 初始化失败，死循环
  }

  // 设置文字大小和颜色
  display.setTextSize(2);       // 2倍大小
  display.setTextColor(SSD1306_WHITE); // 白字
  display.dim(true);
}

#define WIFI_DISCONNECTED (WiFi.status() != WL_CONNECTED)
const char *ssid = "Mk-wifi";
const char *password = "Y6SXeuMDReX $rx@[KRm";

std::atomic<bool> ResetWifi(false);

void reset_wifi(void *) {
  for (;;) {
    if (ResetWifi.load()) {
      WiFi.disconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      ResetWifi.store(false);
    } else {
      delay(500);
    }
  }
}

void spawn_wifi_task() {
  xTaskCreate(reset_wifi, "wifi_reset", 2000, NULL, ESP_TASK_PRIO_MAX - 1, NULL);
}

void connect_wifi() {
  while (WIFI_DISCONNECTED) {
    ResetWifi.store(true);
    
    int times = 0;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Conn WiFi");
    while (++times < 20 && WIFI_DISCONNECTED) {
      display.print('.');
      display.display();

      start_flash_light(250, 1);
      delay(500);
    }
  }

  Serial.print("LAN IP: ");
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
  long elapsed =
      ((unsigned long)esp_timer_get_time() - startTime) / 1000;

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

const long VALUE_MISSING = -1;
std::atomic<long> Elapsed(VALUE_MISSING);

const int BITS_OF_STATUS = 3;
typedef uint32_t recenterror_t;

std::atomic<recenterror_t> RecentError(0);
const recenterror_t EMPTY = 0;
const recenterror_t STATUS_MASK = 0b111;
const recenterror_t REQUEST_SUCCEED = STATUS_MASK;
const recenterror_t REQUEST_FAILED = 0b001;
const recenterror_t REQUEST_TIMEOUT = 0b010;
const recenterror_t REQUEST_TIMEOUT_LONG = 0b011;
const recenterror_t REQUEST_TIMEOUT_MESSY = 0b100;

void maimai_check_worker(void *) {
  recenterror_t recent_error;

  for (;;) {
    if (WIFI_DISCONNECTED) {
      delay(100);
      continue;
    }
    long elapsed = maimai_check();
    Elapsed.store(elapsed);

    recent_error <<= BITS_OF_STATUS;
    if (elapsed <= 0) recent_error |= REQUEST_FAILED;
    else if (elapsed >= 4000) recent_error |= REQUEST_TIMEOUT_MESSY;
    else if (elapsed >= 2000) recent_error |= REQUEST_TIMEOUT_LONG;
    else if (elapsed >= 1000) recent_error |= REQUEST_TIMEOUT;
    else recent_error |= REQUEST_SUCCEED;
    RecentError.store(recent_error);

    if (elapsed > 0 && elapsed < 1000) {
      delay(500);
    } else {
      delay(2000);
    }
  }
}

void spawn_maimai_check() {
  xTaskCreate(maimai_check_worker, "maimai_check", 8000, NULL, ESP_TASK_PRIO_MAX / 2, NULL);
}

void setup() {
  Serial.begin(115200);

  spawn_flash_task();
  initialise_oled();
  
  spawn_wifi_task();
  connect_wifi();

  maimai_check_setup();
  spawn_maimai_check();
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);

  recenterror_t recent_errors = RecentError.load();
  for (int i=9; i>=0; i--) {
    recenterror_t is_error = ( recent_errors >> (BITS_OF_STATUS*i) ) & STATUS_MASK;
    switch (is_error) {
      case REQUEST_FAILED:
        display.print("X");
        break;
      case REQUEST_TIMEOUT:
        display.print("T");
        break;
      case REQUEST_TIMEOUT_LONG:
        display.print("?");
        break;
      case REQUEST_TIMEOUT_MESSY:
        display.print("#");
        break;
      case REQUEST_SUCCEED:
        display.print("O");
        break;
      default:
        display.print(" ");
        break;
    }
  }
  display.print("\n");

  int rssi = WiFi.RSSI();
  if (!rssi && WIFI_DISCONNECTED) {
    Serial.println("Start reconnecting...");
    connect_wifi();
    Serial.println("Reconnected");
    return;
  }
  display.printf("%6d dBm\n", rssi);

  long elapsed = Elapsed.load();
  if (elapsed > 0) {
    display.printf("%6ld ms\n", elapsed);
  } else if (elapsed == VALUE_MISSING) {
    display.printf("%6s ms\n", "--");
  } else {
    display.printf("%6s ms\n SVR DOWN\n", "--");
  }
    
  display.display();

  delay(100);
}

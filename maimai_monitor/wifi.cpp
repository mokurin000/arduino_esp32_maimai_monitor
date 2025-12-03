#include <atomic>

#include "led.hpp"
#include "ssd1306.hpp"
#include "wifi.hpp"

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
  xTaskCreate(reset_wifi, "wifi_reset", 2000, NULL, ESP_TASK_PRIO_MAX - 1,
              NULL);
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

#include "check.hpp"
#include "led.hpp"
#include "ssd1306.hpp"
#include "wifi.hpp"

void setup() {
  Serial.begin(115200);

  spawn_flash_task();
  initialise_oled();

  spawn_wifi_task();
  connect_wifi();

  spawn_maimai_check();
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);

  recenterror_t recent_errors = RecentError.load();
  for (int i = 9; i >= 0; i--) {
    recenterror_t is_error =
        (recent_errors >> (BITS_OF_STATUS * i)) & STATUS_MASK;
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

  display.setTextSize(1);
  display.printf("  DX 1.50  O %8" PRIu32 "\n", SuccCount.load());
  display.printf("T %8" PRIu32 " E %8" PRIu32, TimeoutCount.load(),
                 ErrCount.load());

  display.display();

  delay(100);
}

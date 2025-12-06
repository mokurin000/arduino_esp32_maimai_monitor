#include <time.h>

#include "check.hpp"
#include "led.hpp"
#include "ssd1306.hpp"
#include "wifi.hpp"

// SAFETY: this function is neither reentrant nor thread-safe.
const char *get_localtime(char *buf, size_t buf_len) {
    time_t rawtime;
    // get RTC timer
    time(&rawtime);

    // get local time
    const struct tm *const tzinfo = localtime(&rawtime);
    strftime(buf, buf_len, "%Y-%m-%d %H:%M:%S", tzinfo);
    return buf;
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("Start initialization...");

    spawn_flash_task();
    spawn_wifi_task();

    initialise_oled();
    connect_wifi();

    configTime(3600 * 8,                       // UTC+8:00
               0,                              // DST offset
               "203.107.6.88", "47.96.149.233" // Alibaba NTP
    );

    spawn_maimai_check();
}

void loop(void) {
    display.clearDisplay();
    display.setCursor(0, 0);

    display.setTextSize(1);

    char timebuf[22];
    const char *const localtime = get_localtime(timebuf, sizeof(timebuf));
    display.print(' ');
    display.println(localtime);

    recenterror_t recent_errors = RecentError.load();
    for (int i = 20; i >= 0; i--) {
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

    display.setTextSize(2);

    int rssi = WiFi.RSSI();
    if (!rssi && WIFI_DISCONNECTED) {
        Serial.println("Start reconnecting...");
        connect_wifi();
        Serial.println("Reconnected");
        return;
    }

    long elapsed = Elapsed.load();
    if (elapsed > 0) {
        display.printf("%6ld ms\n", elapsed);
    } else if (elapsed == VALUE_MISSING) {
        display.printf("%6s ms\n", "--");
    } else {
        display.printf("%6s ms\n SVR DOWN\n", "--");
    }

    display.setTextSize(1);
    display.printf("    RSSI: %4d dBm\n", rssi);
    display.printf("    SUCC: %8" PRIu32 "\n", SuccCount.load());
    display.printf("T:%8" PRIu32 " E:%8" PRIu32 "\n", TimeoutCount.load(),
                   ErrCount.load());

    display.println("   maimai DX. 1.50");

    display.display();

    delay(100);
}

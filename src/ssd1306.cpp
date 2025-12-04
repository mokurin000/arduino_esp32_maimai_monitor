#include <Wire.h>

#include "ssd1306.hpp"

// ESP32 硬件 I2C 引脚
#define SCL_PIN 32
#define SDA_PIN 33

#define OLED_ADDR 0x3C

// 创建 I2C 实例（ESP32 默认使用 Wire，也可以自定义）
static TwoWire I2CESP32 = TwoWire(0); // I2C0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2CESP32,
                         -1); // -1 表示不使用 reset 引脚

// initialize `display`
void initialise_oled() {
    // 初始化硬件 I2C（ESP32）
    I2CESP32.begin(SDA_PIN, SCL_PIN, 400000); // 400kHz 速度

    // 初始化 OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // 初始化失败，死循环
    }

    // 设置文字大小和颜色
    display.setTextSize(2);              // 2倍大小
    display.setTextColor(SSD1306_WHITE); // 白字
    display.dim(true);
}

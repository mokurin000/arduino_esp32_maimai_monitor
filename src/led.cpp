#include <Arduino.h>
#include <atomic>
#include <esp_task.h>

#define NO_LED_FLASHING 1

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

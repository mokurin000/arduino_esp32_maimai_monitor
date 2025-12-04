# maimai monitor

> Try it without an ESP32 and a SSD1315 screen: [Wokwi simulation](https://github.com/mokurin000/wokwi-esp32-maimai-monitor)
>
> This is legacy now, but it's for people had official platformio VSCode extension, and who are not going to migrate to `pioarduino`.

**NOW integrated with OLED 128x64 SSD13xx.**

By posting ping request to maimai wahlap server, we could check it's response time (with an existing TLS session) and whether it's responding.

In this project, you could be notified in two ways: the LED at pin 2, and an external OLED screen.

## Galary

![](imgs/OLED.jpg)

## Build with Arduino

Before start, you must have `Adafruit SSD1306` installed.

- Connect the `OLED:VCC` with a 5V or 3.3V power supplyment (according to the vendor)
- Connect `OLED:GND` with `ESP:GND` if you used `3.3V`/`VIN` on the board, or `*:GND` for an external power.
- Connect `ESP:D22` with `OLED:SDA`, and connect `ESP:D23` with `OLED:SDL`.

Now edit the Wifi SSID and password, compile and upload.

## Build with VSCode/pioarduino

To have a better expierence than Arduino IDE, setup `pioarduino`, and reload window, wait for automatic installation.

You should install `C/C++` from Microsoft to be able to find headers without manual configuration, `clangd` is however effortless about parsing the generated `c_cpp_properties.json`.

Once setup pioarduino, you could type `Command+Alt+P` and select `pioarduino: Build`.

## Upload records manually

> This is for partition table changes, testing purpose, e.g.
>
> Uploading the firmware will not overwrite your SPIFFS or LittleFS.

To upload the DX1.50 file, you may have [`arduino-spiffs-upload`](https://github.com/codemee/arduino-spiffs-upload) installed to `~/.arduinoIDE/plugins/`, and type `Command+Alt+P` to run the `Upload SPIFFS...` command.

Alternatively, you could use the `pioarduino: open pio arduino core cli` inside VSCode, then

```bash
pio run -t uploadfs
```

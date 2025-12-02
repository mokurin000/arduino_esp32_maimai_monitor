# maimai monitor

> Try it without an ESP32 and a SSD1315 screen: [Wokwi simulation](https://github.com/mokurin000/wokwi-esp32-maimai-monitor)

**NOW integrated with OLED 128x64 SSD13xx.**

By posting ping request to maimai wahlap server, we could check it's response time (with an existing TLS session) and whether it's responding.

In this project, you could be notified in two ways: the LED at pin 2, and an external OLED screen.

## Function

This is a dedicated **maimai server watchdog** that constantly pings a hidden Chinese server endpoint, shows a 10-check rolling status history on an OLED, screams with fast LED flashing when the server dies, and gives graduated warnings when latency gets bad — all in real time with virtually no delay.

<details>
  <summary>* More detail</summary>

This ESP32 program is a **real-time server availability & latency monitor** for a specific maimai (dance arcade game) server. It continuously checks the server's reachability and response time, and displays the status in two main ways:

#### 1. **On the OLED Screen (Main User Feedback)**
The `loop()` function updates the 128×64 OLED **every 100 ms** with the following information:

```
OOOOOOOOOO      ← 10 latest check results (recent on the right)
   -39 dBm      ← Current WiFi signal strength
    128 ms      ← Latest successful response time
                 (or "-- ms  SVR DOWN" if unreachable)
```

**Meaning of the 10-character status line (from left = oldest → right = newest):**
- `O` = Good response **and** fast (< 1000 ms)
- `T` = Good response but **slow** (≥ 1000 ms) → "Timeout-ish" / high latency
- `X` = Failed to reach server (timeout, no response, connection error)

So you get a **10-check rolling history** that instantly shows if the server is stable, intermittently down, or consistently slow.

#### 2. **Via the Built-in LED (Physical Alert)**
The onboard LED flashes in different patterns depending on the situation:

| Situation                          | Flash Pattern                     | Meaning |
|------------------------------------|-----------------------------------|-------|
| WiFi connecting                    | Single quick flash every 0.5 s    | Trying to connect |
| Server completely unreachable      | 15× very fast flashes (100 ms)    | **Critical alert** – server is down |
| Response ≥ 4000 ms                 | 3× slow flashes (2 s on/off)      | Extremely high latency |
| Response 2000–3999 ms              | 4× medium flashes (1 s)           | High latency |
| Response 1000–1999 ms              | 8× faster flashes (500 ms)        | Noticeably slow |
| Response < 1000 ms                 | No flashing (steady off)          | Everything is fine |

The flashing task runs independently and is non-blocking.

#### 3. **Serial Monitor (for debugging)**
Prints detailed logs, IP address, response times, and error messages.

---

### Main Real Logic Summary

```cpp
1. setup()
   ├─ Start LED flashing task
   ├─ Initialize OLED
   ├─ Connect to WiFi (with dots + flashing feedback)
   ├─ Start background maimai_check_worker task
   └─ Configure HTTPS client (insecure, custom headers, fixed URL)

2. maimai_check_worker (runs forever in background)
   → Every few seconds:
        • Sends a fixed 16-byte binary POST request to 
          https://152.136.99.118:42081/... (bypassing certificate check)
        • Measures exact round-trip time in milliseconds
        • Stores latest elapsed time → Elapsed atomic var
        • Updates RecentError bitfield:
            - Bit 0 (0x01) = unreachable this check
            - Bit 4 (0x10) = slow (≥1000 ms) this check
        • Triggers appropriate LED flash pattern on slow/unreachable
        • Sleeps 0.5 s if fast, 2 s if slow/down (adaptive polling)

3. loop() – UI thread (runs on core 1, very lightweight)
   → Every 100 ms:
        • Decodes the last 10 checks from RecentError bitfield
        • Prints O / T / X history row
        • Shows WiFi RSSI
        • Shows latest latency or "SVR DOWN"
        • Refreshes OLED
```

### Key Technical Details

- Uses **FreeRTOS tasks**: one for LED flashing, one for continuous server checks → completely non-blocking
- **Atomic variables** for safe cross-task communication
- **Bit-packing** of the last 10 results into a single `unsigned int` (2 bits per check → 20 bits total, last 10 shown)
- Insecure HTTPS (`setInsecure()`) because the real server uses a self-signed cert
- Fixed binary payload and many custom headers to perfectly mimic the official maimai DX client's request
</details>

## Pictures

![](imgs/OLED.jpg)

## Build

Before start, you must have `Adafruit SSD1306` installed.

- Connect the `OLED:VCC` with a 5V or 3.3V power supplyment (according to the vendor)
- Connect `OLED:GND` with `ESP:GND` if you used `3.3V`/`VIN` on the board, or `*:GND` for an external power.
- Connect `ESP:D22` with `OLED:SDA`, and connect `ESP:D23` with `OLED:SDL`.

Now edit the Wifi SSID and password, compile and upload.

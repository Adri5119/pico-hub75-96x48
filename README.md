# 96x48 LED Panel Driver for Raspberry Pi Pico — HUB75 SM5368

A small Arduino-Pico library for drawing pixels, text, shapes, and simple animations on a **96x48 LED panel (96×48 RGB LED matrix)** with the **HUB75 interface and SM5368 row-selection protocol**. It targets the Waveshare P2.5 flexible 96x48 panel, revision 24S-A1, with a Raspberry Pi Pico W / RP2040. It provides an Adafruit GFX canvas, a 2,304-byte packed framebuffer, configurable GPIOs, and a deliberately simple software scan loop.

Developed with AI assistance from **OpenAI ChatGPT / Codex**; see [AI assistance](#ai-assistance) for the contribution details.

This grew out of bringing up a panel that showed repeated horizontal bands with ordinary binary HUB75 row addressing. The important part is the **row-selection protocol**, not just setting a library's resolution to 96×48.

## Supported hardware and project status

The target is a Raspberry Pi **Pico W / RP2040**, using Earle Philhower's **Arduino-Pico** core, and this panel:

```text
Waveshare RGB-Matrix-P2.5-96x48-F
SOFT-P2.5-96x48-1515-24s-A1
96 × 48, 1/24 scan, SM5368 row control
```

Waveshare identifies **24S-A1 as V2** and **24S-A2.1 as V1**. See the [panel documentation](https://docs.waveshare.com/RGB-Matrix-Px-96x48). The V1 revision and other panels with the same resolution are **not verified by this project**. This is not a universal 96×48 driver. Confirm the PCB revision and row-control hardware before choosing it.

The underlying scan sequence and pixel mapping were used successfully on a physical Pico W and the panel above in the original application. This extracted library has passed automated framebuffer/GPIO-trace checks and PlatformIO compilation for Pico W and Pico. **The extracted example has not yet been visually revalidated on a physical panel.** Plain Pico support is compile-tested only. Arduino IDE instructions are provided, but an Arduino IDE build has not been tested. No oscilloscope or logic-analyzer measurements are claimed.

No panel chaining, alternate row protocols, ESP32 port, RP2350/Pico 2 support, PIO/DMA engine, Wi-Fi controller, or phone application is included.

## What it does

- Addresses all **4,608 pixels** through 24 paired scan positions, with independent upper and lower RGB data.
- Draws through **Adafruit GFX**: pixels, lines, rectangles, circles, fonts, and text.
- Supports GFX rotations 0–3 and clips out-of-range pixel writes.
- Provides eight native colors: black plus seven combinations of RGB.
- Controls global brightness from 0–100% by scaling row illumination time.
- Optionally approximates RGB888 colors with a fixed **4×4 spatial dither**. Adjacent pixels mix the requested shade; individual LEDs still have one bit per color.
- Includes a two-core demonstration with USB commands for row and pixel diagnostics. USB input and drawing never operate on the framebuffer concurrently.

The library starts no background task. Your program must call `refresh()` repeatedly. The example dedicates core 1 to drawing and scanning, leaving core 0 for USB commands.

## Hardware and wiring

You need the compatible panel, a Pico or Pico W, a data-capable USB cable, a suitable HUB75 adapter or correctly wired logic-level interface, a ribbon cable, and an external regulated **5 V supply**. Waveshare lists this panel at **5 V / 4 A**; check polarity and your particular panel's requirements in its [specification](https://docs.waveshare.com/RGB-Matrix-Px-96x48).

Power the matrix through its power connector, **not through a Pico GPIO or the Pico's 3.3 V supply**. Connect the controller/interface and panel grounds. Use the panel's **HUB75 input**, not its output connector. Disconnect power before changing connections. The Pico's GPIOs are 3.3 V and are not 5 V tolerant; use an appropriate adapter/level shifter for the panel's input requirements. This repository does not certify a particular adapter.

### Edit the example pin map first

[`examples/BasicDemo/PanelConfig.h`](examples/BasicDemo/PanelConfig.h) contains an **arbitrary, explicit wiring example**. It is not a commercial adapter's pinout. Numbers below are **GPIO / GP numbers**, not physical header pin numbers. Trace your adapter or consult its schematic and replace the values before uploading.

| Panel signal | Example Pico GPIO | Purpose |
|---|---:|---|
| R1 | GP2 | Upper half red |
| G1 | GP3 | Upper half green |
| B1 | GP4 | Upper half blue |
| R2 | GP5 | Lower half red |
| G2 | GP6 | Lower half green |
| B2 | GP7 | Lower half blue |
| CLK | GP8 | RGB shift clock |
| LAT / STB | GP9 | RGB latch |
| OE | GP10 | Active-low output enable |
| A | GP11 | SM5368 row clock |
| B | GP12 | SM5368 BK control |
| C | GP13 | SM5368 row serial data |
| D, E | Unused | Not driven by this driver |
| GND | Common ground | Controller/interface and panel ground |

The struct order is `R1,G1,B1,R2,G2,B2,CLK,LAT,OE,A,B,C`. Pins must be distinct and from exposed GP0–22 or GP26–28. `begin()` rejects duplicates and unavailable GPIOs before touching the hardware. It cannot detect a correctly numbered but incorrectly wired pin.

**Color order varies.** The `r1`/`r2` entries mean the GPIOs that actually illuminate red, not a guarantee about a panel connector's printed labels. If red and blue are swapped, exchange the R/B entries in both halves of `PanelConfig.h`. Keep the scanning code unchanged. Do not substitute an ordinary A/B/C binary address sequence.

## Quick start with VS Code and PlatformIO

1. Install VS Code, the PlatformIO IDE extension, and Git. Clone this repository, or download its source ZIP and extract it.
2. Open this repository's root folder in VS Code. Read the hardware section and edit `examples/BasicDemo/PanelConfig.h` to match your wiring.
3. In PlatformIO's sidebar, expand **Project Tasks → pico_w → General**. Select **Build**. For a non-wireless RP2040 Pico, use the `pico` environment instead.
4. Connect the Pico with a USB data cable and select **Upload**. If it cannot enter upload mode, disconnect USB, hold **BOOTSEL**, reconnect USB, then release BOOTSEL and retry. Alternatively, copy your freshly built `.pio/build/pico_w/firmware.uf2` to the `RPI-RP2` USB drive. Use the matching `pico` output when applicable.
5. Power the panel externally. The example starts with a diagnostic grid at **25% brightness**. Open **Monitor** at 115200 baud, choose a line ending of **LF** or **CRLF**, and send `help` followed by Enter.

Equivalent commands in a PlatformIO terminal:

```sh
pio run -e pico_w
pio run -e pico_w -t upload
pio device monitor -b 115200
```

Use `-e pico` to build/upload for a plain Pico. The project pins the maxgerhardt platform to commit `aa70b802be8851668053d4f09734e4089fe41932`, selecting Arduino-Pico 5.6.1 (`832f2c06`), and Adafruit GFX 1.13.0. First builds download the toolchain and dependencies and may take several minutes. The official Arduino Mbed core is not the supported core for this example.

No credentials, network configuration, API account, or phone is needed. The repository distributes source; generated firmware is built locally for your selected board and pins.

### USB diagnostic commands

| Command | Result |
|---|---|
| `help` | Prints commands and color numbers |
| `grid` | Colored border, diagonals, and binary row markers |
| `text` | Displays “96 x 48” and “SM5368” |
| `blank` | Clears the framebuffer; scanning continues |
| `pixel 0 0 7` | Clears the display and lights one white pixel at x=0, y=0 |
| `pixel 95 47 4` | Clears the display and lights one blue pixel at the opposite corner |
| `row 0` | Full-width red row 0 and green row 24 |
| `row 23` | Full-width red row 23 and green row 47 |
| `solid 1` | Fills with red; use color numbers 0–7 |
| `brightness 10` | Sets 10% global brightness |
| `brightness 0` | Blanks output without stopping scan progression |

Coordinates use zero-based x=0–95 and y=0–47 at rotation 0. Color numbers are **black 0, red 1, green 2, yellow 3, blue 4, magenta 5, cyan 6, white 7**. The example accepts one command per line, up to 95 characters. It queues at most four commands and applies one per scan. “Queued” acknowledges queue insertion; malformed or out-of-range commands do not change the display. Core 1 owns the framebuffer; core 0 owns Serial. This is a pixel diagnostic, not a network-control service.

## Use it in your own project

### PlatformIO dependency

Use the same Arduino-Pico platform/core selection as `platformio.ini` and add this repository to your own project's dependencies:

```ini
lib_deps =
    https://github.com/Adri5119/pico-hub75-96x48.git
```

For repeatable builds, append `#<commit-sha>` to the library URL once you have chosen a version. The library manifest brings in Adafruit GFX. Include `<Hub75_96x48.h>` and construct a canvas with your own pin map. The root `platformio.ini` builds the bundled demonstration; it is not required in applications that use the library as a dependency.

### Minimal single-core sketch

This example illustrates the API. **Replace its GPIO map** to match your hardware. Constantly calling `refresh()` is sufficient for a static image; a `delay()` between frames will cause flicker.

```cpp
#include <Arduino.h>
#include <Hub75_96x48.h>

constexpr Hub75_96x48::Pins pins = {
  2,3,4, 5,6,7, 8,9,10, 11,12,13
};
Hub75_96x48 panel(pins);  // Keep the framebuffer out of a small stack.

void setup() {
  if (!panel.begin()) while (true) delay(1000);
  panel.setBrightness(25);
  panel.clear();
  panel.drawPixel(95, 47, Hub75_96x48::Blue);
  panel.drawRect(0, 0, 96, 48, Hub75_96x48::Green);
  panel.setTextWrap(false);
  panel.setTextColor(Hub75_96x48::White);
  panel.setCursor(12, 20);
  panel.print("HELLO!");
}

void loop() {
  panel.refresh();
}
```

GFX drawing colors are **native 0–7 bitmasks, not RGB565**. Do not pass a TFT-style `0xF800` for red. `readPixel(x,y)` returns the native color and respects rotation. `setRotation(1)` or `setRotation(3)` gives a logical canvas of 48×96; rotations 0 and 2 give 96×48.

For spatially mixed colors:

```cpp
panel.drawRgbPixel(20, 20, 255, 128, 0); // Approximate orange over a pixel area.
panel.setTextRgb888(255, 100, 180);
panel.setCursor(2, 2);
panel.print("COLOR");
```

A single pixel can only be one of the eight native colors. Dithering approximates RGB intensity across neighboring pixels, so broad shapes look smoother than small text. It adds no PWM planes or extra scan stages. To dither an area, draw each of its pixels with `drawRgbPixel`, or set the text RGB value and use `Hub75_96x48::RgbText` as a GFX shape color. `setTextColor(nativeColor)` restores ordinary text drawing.

### Arduino IDE

Install Earle Philhower's [Arduino-Pico core](https://github.com/earlephilhower/arduino-pico) using its installation instructions. Install **Adafruit GFX Library** and its dependencies through Library Manager. Install this repository as a ZIP library through **Sketch → Include Library → Add .ZIP Library**, then open **File → Examples → Pico HUB75 96x48 SM5368 → BasicDemo**. Select the matching Pico/Pico W board from the Arduino-Pico package, edit `PanelConfig.h`, and upload. The `.ino` file is an entry point; the shared implementation is in `Demo.cpp`. These IDE steps have not been tested in this extraction.

## How the scan works

The SM5368 interface uses **A as row clock, B as BK, and C as row serial data**, instead of an ordinary parallel binary row address. Waveshare's [SM5368 notes](https://github.com/waveshareteam/RGB-Matrix-Px-xx/blob/main/example/README.md) describe this distinction and the separate R/B mapping issue. Waveshare also provides its own [Pico examples and instructions](https://docs.waveshare.com/RGB-Matrix-Px-96x48/Raspberry-Pico); this project is a small alternative with an Arduino GFX interface, not the only software for these panels.

This implementation:

1. Raises OE before enabling its GPIO output, then initializes other selected GPIOs low.
2. Shifts 32 zero row bits while blanked to clear stale row selections at startup.
3. For each of 24 scan positions, shifts 96 six-bit RGB values with OE high. RGB1 represents y=row; RGB2 represents y=row+24.
4. Pulses LAT, then advances the row clock. C is high only for position 0 of each frame; B stays high during row clocks.
5. Lowers OE for `100 µs × brightness / 100`, raises OE again, and proceeds to the next position. At brightness zero, OE remains high throughout.
6. Leaves OE high and pads the complete scan to at least **8,000 µs**, including shifting and illumination time. All 24 positions advance on every frame, even if the image is black.

RGB data setup and clock-high waits are each 1 µs. Latch setup, high, and hold waits are 1 µs each. Row-clock setup and high waits are 1 µs each, followed by a 2 µs blanking wait before enabling LEDs. These are software delay requests; real instruction and GPIO overhead adds time. The default minimum frame period corresponds to a maximum of about **125 frames/s**, not a measured guaranteed refresh rate. Interrupts or other work can lengthen a frame.

The framebuffer stores `frame[24][96]`: upper RGB in bits 0–2, lower RGB in bits 3–5, with red as the lowest bit in each half. It uses 2,304 bytes; a 64-entry GPIO lookup uses another 256 bytes. `framebuffer()` exposes only a const pointer for inspection. Drawing and clearing change memory; they do not immediately refresh the hardware.

## Integrating with networking, sensors, or storage

`refresh()` is blocking and takes at least 8 ms. It does not use PIO, DMA, timers, or interrupt handlers to scan automatically. The bundled example uses Arduino-Pico's [`setup1()` / `loop1()` multicore support](https://arduino-pico.readthedocs.io/en/latest/multicore.html), with a bounded Pico SDK queue carrying commands from core 0.

**One core must own every call to the panel**, including drawing, `begin()`, `refresh()`, rotation, and brightness. The framebuffer is not double-buffered and the class is not thread-safe. For networking on core 0, pass small validated commands or snapshots through a queue to core 1. Do not draw from a Wi-Fi callback while the other core is scanning. Avoid blocking Serial, network calls, allocation, or long rendering tasks on the display core. Large redraws between scans can still reduce refresh frequency.

**Coordinate flash writes explicitly.** Arduino-Pico can pause the other core during flash erase/write operations, which could freeze OE low in mid-scan. Before EEPROM/LittleFS writes or other deliberate core pauses, request that the display owner complete a frame and acknowledge a paused state while OE is high. Only then perform the write; resume scanning afterward. This library has no persistence or automatic flash-write handshake. Do not add flash writes to the example without one. The same care applies to breakpoints or any operation that can halt scanning with a row illuminated.

## Troubleshooting and validation

| Symptom | Checks |
|---|---|
| Completely dark | External 5 V supply/polarity, common ground, input connector, OE wiring, configured pins; `brightness 25`, then `grid` |
| Repeated bands / only some rows | Confirm 24S-A1/SM5368 hardware; check A/B/C signal roles and pin map; D/E are not binary address bits here |
| Red and blue exchanged | Swap logical R/B GPIO entries for both RGB halves |
| Image wraps or short lines | Check all 96 RGB clock pulses, CLK/LAT wiring, ribbon seating, and the panel revision; use `row 0` and `row 23` |
| Duplicate/smeared pixels | Check input connector, cable/ground quality, level shifting, and whether another core or interrupt is touching the same pins |
| Serial command has no effect | Send LF/CRLF; check `help`, exact command syntax, ranges, and the correct USB port; close other monitors using that port |
| Flicker after adding features | Ensure continuous refresh; move blocking work off the display core; coordinate flash writes |
| Invalid pin map | `begin()` returns false and the example stops its display core; check for duplicates and non-exposed GPIOs |

For a new panel, confirm borders and diagonals with `grid`, check red/green/blue with `solid 1`, `solid 2`, `solid 4`, and walk `row 0` through `row 23`. Each `row N` must give exactly two full-width lines separated by 24 rows. Test individual corner pixels and y=23/24 to confirm the half boundary. Verify brightness zero, startup/reset behavior, and prolonged operation. These physical checks remain necessary even when compilation and simulated traces pass.

### Automated checks

```sh
python scripts/test_host.py
pio run -e pico_w -e pico
```

Host checks need g++/clang++, or `python -m pip install ziglang`. They compile the actual driver against small GPIO/time and GFX test doubles. Tests cover every pixel and rotation, half-row packing, clipping, pin validation, RGB output mapping, 96 clocks per latch, all 24 row stages, first-row injection, OE blanking, brightness dwell, and the minimum frame period. They verify the requested software sequence, **not electrical behavior, color calibration, or the real panel's response**. Firmware builds use the real Adafruit GFX dependency. GitHub Actions runs these checks for submitted changes; a passing workflow is not a hardware certification.

## AI assistance

This project was developed with substantial AI assistance from **OpenAI ChatGPT / Codex**. The AI assistant helped investigate the SM5368 row-scanning behavior, write and refactor driver code, create drawing and diagnostic examples, develop automated checks, and prepare the documentation and public repository.

The maintainer supplied the hardware, panel identification, requirements, and observations from physical testing. The verification status and remaining physical-panel checks are documented above. This acknowledgment credits the AI-assisted development process; the project's original code remains under the MIT license, and upstream contributors retain the credits and licenses listed below.

## License, credits, and contributing

Original driver, examples, tests, and documentation are available under the [MIT License](LICENSE). You may reuse them in your own projects while retaining the license notice. Dependencies keep their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Thanks to **Waveshare** for panel documentation and the SM5368 protocol notes; **Adafruit** for GFX, its drawing API and fonts; **Earle Philhower and Arduino-Pico contributors** for the RP2040 Arduino core; **Raspberry Pi** for the Pico SDK; and **Maximilian Gerhardt / PlatformIO contributors** for the platform integration. This is an independent community project, not an official Waveshare, Raspberry Pi, or Adafruit product.

For a compatibility report, include the exact panel marking/revision, row-driver marking if known, controller/core version, adapter or wiring map, and what the diagnostic commands display. A new panel revision should get its own documented hardware result rather than a blanket claim that all 96×48 panels work. Never include Wi-Fi passwords, API keys, or private application files in an issue or pull request.

# Attribution and dependency licenses

This repository's original code and documentation are MIT-licensed; that grant does not change the licenses of dependencies, toolchains, or external examples.

## AI-assisted development

**OpenAI ChatGPT / Codex** assisted with row-scanning investigation, implementation and refactoring, diagnostic examples, automated checks, documentation, and preparation of this public repository. The maintainer provided the hardware and physical-test observations. See the [README's AI assistance acknowledgment](README.md#ai-assistance) and its hardware-verification status.

## Upstream components and references

| Component / reference | Use here | Upstream license / attribution |
|---|---|---|
| [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) | Downloaded dependency; base canvas, drawing, text, fonts | BSD-style, copyright (c) 2012 Adafruit Industries. See the retained [license text](licenses/Adafruit-GFX-BSD.txt). |
| [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) | Transitive GFX dependency managed by PlatformIO / Library Manager | MIT; retain its upstream license with redistributions. |
| [Arduino-Pico](https://github.com/earlephilhower/arduino-pico) | Framework dependency; core, multicore entry points and build support | Primarily LGPL-2.1; bundled components have additional licenses. Consult the [upstream LICENSE](https://github.com/earlephilhower/arduino-pico/blob/master/LICENSE) and individual source notices. |
| [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) | GPIO, time, and queue APIs through Arduino-Pico | BSD-3-Clause for the SDK's original code, with separately licensed bundled components. See [LICENSE.TXT](https://github.com/raspberrypi/pico-sdk/blob/master/LICENSE.TXT). |
| [maxgerhardt/platform-raspberrypi](https://github.com/maxgerhardt/platform-raspberrypi) / [PlatformIO](https://github.com/platformio/platform-raspberrypi) | Downloaded build platform | Apache-2.0; see the [platform license](https://github.com/maxgerhardt/platform-raspberrypi/blob/develop/LICENSE). |
| [Waveshare panel documentation](https://docs.waveshare.com/RGB-Matrix-Px-96x48) and [SM5368 notes](https://github.com/waveshareteam/RGB-Matrix-Px-xx/blob/main/example/README.md) | Hardware identification and protocol reference | Credited as references, not relicensed under this project's MIT license. No Waveshare example source, images, or schematics are bundled. |

The driver was extracted and refactored from a custom application that used this panel successfully. It preserves that application's row-clock approach, paired pixel layout, and conservative software delay sequence. The host test doubles are original minimal implementations, not copies of Adafruit or Raspberry Pi source. The 4×4 ordered-dither arrangement is the conventional Bayer threshold pattern.

The example is built with upstream dependencies, not a vendored graphics or panel library. Adafruit Protomatter was used during earlier panel investigation but is not used or bundled in this library. The independent driver must not be represented as an Adafruit Protomatter port.

Only source is published here. If you redistribute compiled firmware, review the actual linked components and include all required notices and any corresponding-source/relinking materials required by their licenses. Do not assume this repository's MIT license covers the Arduino core, its bundled libraries, or your toolchain runtime. Names and trademarks belong to their respective owners; no affiliation or endorsement is claimed.

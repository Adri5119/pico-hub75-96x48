// SPDX-License-Identifier: MIT
#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

// One 96x48, 1/24-scan panel using the SM5368 A/B/C row protocol.
// RP2040 + Earle Philhower Arduino-Pico. Call all methods on one core.
class Hub75_96x48 : public Adafruit_GFX {
 public:
  struct Pins {
    uint8_t r1, g1, b1, r2, g2, b2;
    uint8_t clk, lat, oe;
    uint8_t a, b, c;  // SM5368 row clock, BK, serial row data.
  };
  enum Color : uint16_t {
    Black = 0, Red = 1, Green = 2, Yellow = 3,
    Blue = 4, Magenta = 5, Cyan = 6, White = 7
  };
  static constexpr int16_t PanelWidth = 96, PanelHeight = 48;
  static constexpr uint32_t FramePeriodUs = 8000;
  static constexpr uint32_t MaxRowOnUs = 100;
  static constexpr uint16_t RgbText = 0x100;

  explicit Hub75_96x48(const Pins& pins);
  // Returns false without touching GPIO if any pin is invalid or duplicated.
  bool begin();
  // Blocking full-frame scan (at least 8 ms). Repeat continuously.
  // OE is high when this returns. No background refresh is started.
  void refresh();
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  uint8_t readPixel(int16_t x, int16_t y) const;
  void fillScreen(uint16_t color) override;
  void clear() { fillScreen(Black); }
  // Global row dwell scaling. Zero blanks while continuing the scan.
  void setBrightness(uint8_t percent);
  uint8_t brightness() const { return brightness_; }
  // RGB888 is spatially approximated, not true per-pixel PWM color.
  void drawRgbPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  void setTextRgb888(uint8_t r, uint8_t g, uint8_t b);
  static uint8_t dither(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
  bool ready() const { return ready_; }
  // Read-only physical scan order: [y % 24][x], RGB1 bits 0..2, RGB2 bits 3..5.
  const uint8_t* framebuffer() const { return &frame_[0][0]; }

 private:
  bool physical(int16_t& x, int16_t& y) const;
  void clockRow(bool first);
  Pins pins_;
  uint8_t frame_[24][96] = {};
  uint32_t rgbLut_[64] = {};
  uint32_t rgbMask_ = 0, rowMask_ = 0;
  uint8_t brightness_ = 25;
  uint8_t textR_ = 255, textG_ = 255, textB_ = 255;
  bool ready_ = false;
};

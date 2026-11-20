// SPDX-License-Identifier: MIT
// Minimal test double, not Adafruit source. Firmware uses the real dependency.
#pragma once
#include <stdint.h>
class Adafruit_GFX {
 public:
  Adafruit_GFX(int16_t w, int16_t h) : w_(w), h_(h), width_(w), height_(h) {}
  virtual ~Adafruit_GFX() = default;
  virtual void drawPixel(int16_t, int16_t, uint16_t) = 0;
  virtual void fillScreen(uint16_t) = 0;
  int16_t width() const { return width_; }
  int16_t height() const { return height_; }
  uint8_t getRotation() const { return rotation_; }
  void setRotation(uint8_t r) {
    rotation_ = r & 3;
    width_ = (rotation_ & 1) ? h_ : w_;
    height_ = (rotation_ & 1) ? w_ : h_;
  }
  void setTextColor(uint16_t c) { textColor = c; }
  uint16_t textColor = 0;
 private:
  int16_t w_, h_, width_, height_;
  uint8_t rotation_ = 0;
};

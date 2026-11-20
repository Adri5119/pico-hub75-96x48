// SPDX-License-Identifier: MIT
#include "Hub75_96x48.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <string.h>

namespace {
constexpr uint32_t gpioBit(unsigned pin) { return 1u << pin; }
bool exposed(uint8_t pin) { return pin <= 22 || (pin >= 26 && pin <= 28); }
}

Hub75_96x48::Hub75_96x48(const Pins& pins)
    : Adafruit_GFX(PanelWidth, PanelHeight), pins_(pins) {}

bool Hub75_96x48::begin() {
  if (ready_) return true;
  const uint8_t all[] = {pins_.r1, pins_.g1, pins_.b1, pins_.r2, pins_.g2,
    pins_.b2, pins_.clk, pins_.lat, pins_.oe, pins_.a, pins_.b, pins_.c};
  uint32_t used = 0;
  for (uint8_t pin : all) {
    if (!exposed(pin) || (used & gpioBit(pin))) return false;
    used |= gpioBit(pin);
  }
  const uint8_t rgb[] = {pins_.r1, pins_.g1, pins_.b1, pins_.r2, pins_.g2, pins_.b2};
  for (uint8_t pin : rgb) rgbMask_ |= gpioBit(pin);
  rowMask_ = gpioBit(pins_.a) | gpioBit(pins_.b) | gpioBit(pins_.c);
  for (unsigned value = 0; value < 64; ++value)
    for (unsigned channel = 0; channel < 6; ++channel)
      if (value & gpioBit(channel)) rgbLut_[value] |= gpioBit(rgb[channel]);

  // Set the OE output latch before enabling its output direction.
  gpio_init(pins_.oe);
  gpio_put(pins_.oe, true);
  gpio_set_dir(pins_.oe, GPIO_OUT);
  const uint32_t others = used & ~gpioBit(pins_.oe);
  gpio_init_mask(others);
  gpio_put_masked(others, 0);
  gpio_set_dir_out_masked(others);
  // Clear all possible stale row bits with the panel blanked.
  for (unsigned i = 0; i < 32; ++i) clockRow(false);
  ready_ = true;
  return true;
}

void Hub75_96x48::clockRow(bool first) {
  // SM5368: A is a clock, B stays high, C injects the first-row token.
  gpio_put_masked(rowMask_, gpioBit(pins_.b) | (first ? gpioBit(pins_.c) : 0));
  busy_wait_us_32(1);
  gpio_put(pins_.a, true);
  busy_wait_us_32(1);
}

void Hub75_96x48::refresh() {
  if (!ready_) return;
  const uint64_t start = time_us_64();
  const uint32_t onUs = MaxRowOnUs * brightness_ / 100;
  gpio_put(pins_.oe, true);
  for (unsigned row = 0; row < 24; ++row) {
    for (unsigned x = 0; x < 96; ++x) {
      gpio_put_masked(rgbMask_, rgbLut_[frame_[row][x]]);
      busy_wait_us_32(1);
      gpio_put(pins_.clk, true);
      busy_wait_us_32(1);
      gpio_put(pins_.clk, false);
    }
    busy_wait_us_32(1);
    gpio_put(pins_.lat, true);
    busy_wait_us_32(1);
    gpio_put(pins_.lat, false);
    busy_wait_us_32(1);
    clockRow(row == 0);
    busy_wait_us_32(2);
    if (onUs) {
      gpio_put(pins_.oe, false);
      busy_wait_us_32(onUs);
      gpio_put(pins_.oe, true);
    }
  }
  const uint64_t elapsed = time_us_64() - start;
  if (elapsed < FramePeriodUs) busy_wait_us_32(FramePeriodUs - elapsed);
}

bool Hub75_96x48::physical(int16_t& x, int16_t& y) const {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return false;
  int16_t t;
  switch (getRotation()) {
    case 1: t = x; x = PanelWidth - 1 - y; y = t; break;
    case 2: x = PanelWidth - 1 - x; y = PanelHeight - 1 - y; break;
    case 3: t = x; x = y; y = PanelHeight - 1 - t; break;
  }
  return true;
}

void Hub75_96x48::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!physical(x, y)) return;
  if (color == RgbText) color = dither(x, y, textR_, textG_, textB_);
  const unsigned shift = y < 24 ? 0 : 3;
  uint8_t& pixel = frame_[y % 24][x];
  pixel = (pixel & ~(7u << shift)) | ((color & 7u) << shift);
}

uint8_t Hub75_96x48::readPixel(int16_t x, int16_t y) const {
  if (!physical(x, y)) return Black;
  return (frame_[y % 24][x] >> (y < 24 ? 0 : 3)) & 7u;
}

void Hub75_96x48::fillScreen(uint16_t color) {
  if (color == RgbText) {
    for (int16_t y = 0; y < height(); ++y)
      for (int16_t x = 0; x < width(); ++x) drawPixel(x, y, color);
  } else {
    const uint8_t rgb = color & 7u;
    memset(frame_, rgb | (rgb << 3), sizeof(frame_));
  }
}

void Hub75_96x48::setBrightness(uint8_t percent) {
  brightness_ = percent > 100 ? 100 : percent;
}

uint8_t Hub75_96x48::dither(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  static constexpr uint8_t order[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  const unsigned threshold = order[y & 3][x & 3] * 16u + 7u;
  return (r > threshold ? Red : 0) | (g > threshold ? Green : 0) |
         (b > threshold ? Blue : 0);
}

void Hub75_96x48::drawRgbPixel(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
  // Use physical coordinates for a fixed dither pattern across rotations.
  int16_t px = x, py = y;
  if (physical(px, py)) drawPixel(x, y, dither(px, py, r, g, b));
}

void Hub75_96x48::setTextRgb888(uint8_t r, uint8_t g, uint8_t b) {
  textR_ = r; textG_ = g; textB_ = b;
  setTextColor(RgbText);
}

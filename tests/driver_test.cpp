// SPDX-License-Identifier: MIT
#include "Hub75_96x48.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

struct Event { uint64_t time; uint32_t state; unsigned pin; bool value; };
std::vector<Event> events;
uint64_t now = 0;
uint32_t state = 0;
unsigned calls = 0;
constexpr Hub75_96x48::Pins pins = {2,3,4,5,6,7,8,9,10,11,12,13};
constexpr uint32_t bit(unsigned p) { return 1u << p; }
void check(bool ok, const char* message) {
  if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
void gpio_init(unsigned) { ++calls; }
void gpio_init_mask(uint32_t) { ++calls; }
void gpio_set_dir(unsigned pin, bool out) {
  ++calls;
  if (pin == pins.oe && out) check(state & bit(pin), "OE latch high before output enabled");
}
void gpio_set_dir_out_masked(uint32_t) { ++calls; }
void gpio_put(unsigned pin, bool value) {
  ++calls;
  state = (state & ~bit(pin)) | (value ? bit(pin) : 0);
  events.push_back({now, state, pin, value});
}
void gpio_put_masked(uint32_t mask, uint32_t value) {
  ++calls; state = (state & ~mask) | (value & mask);
}
uint64_t time_us_64() { return now; }
void busy_wait_us_32(uint32_t duration) { now += duration; }

void testFramebuffer() {
  Hub75_96x48 panel(pins);
  check(!panel.ready(), "constructor does not start hardware");
  const unsigned before = calls;
  panel.refresh();
  check(calls == before, "refresh before begin is harmless");
  for (int y = 0; y < 48; ++y)
    for (int x = 0; x < 96; ++x) panel.drawPixel(x, y, (x + 3 * y) % 8);
  for (int y = 0; y < 48; ++y) {
    for (int x = 0; x < 96; ++x) {
      check(panel.readPixel(x, y) == (x + 3 * y) % 8, "all 4608 pixels round trip");
      const uint8_t raw = panel.framebuffer()[(y % 24) * 96 + x];
      check(((raw >> (y < 24 ? 0 : 3)) & 7) == (x + 3 * y) % 8,
            "row halves pack without overwriting each other");
    }
  }
  for (int r = 0; r < 4; ++r) {
    panel.setRotation(r); panel.clear();
    check(panel.width() == (r % 2 ? 48 : 96), "rotated width");
    check(panel.height() == (r % 2 ? 96 : 48), "rotated height");
    for (int y = 0; y < panel.height(); ++y)
      for (int x = 0; x < panel.width(); ++x) panel.drawPixel(x, y, (x + y) % 8);
    for (int y = 0; y < panel.height(); ++y)
      for (int x = 0; x < panel.width(); ++x)
        check(panel.readPixel(x, y) == (x + y) % 8, "every rotated pixel round trips");
    panel.clear(); panel.drawPixel(0, 0, panel.White);
    const int px[] = {0,95,95,0}, py[] = {0,0,47,47};
    const auto raw = panel.framebuffer()[(py[r] % 24) * 96 + px[r]];
    check(raw == (7 << (py[r] < 24 ? 0 : 3)), "rotation origin maps to correct physical corner");
    panel.fillScreen(panel.Cyan);
    panel.drawPixel(-1, 0, 0); panel.drawPixel(0, -1, 0);
    panel.drawPixel(panel.width(), 0, 0); panel.drawPixel(0, panel.height(), 0);
    for (int i = 0; i < 2304; ++i) check(panel.framebuffer()[i] == 54, "clipping is safe");
  }
  unsigned redCount = 0;
  for (int y = 0; y < 4; ++y) for (int x = 0; x < 4; ++x) {
    check(panel.dither(x,y,0,0,0) == 0, "dither black");
    check(panel.dither(x,y,255,255,255) == 7, "dither white");
    redCount += panel.dither(x,y,128,0,0) == panel.Red;
  }
  check(redCount == 8, "half red uses half of a 4x4 tile");
  panel.setRotation(0); panel.clear();
  panel.drawRgbPixel(4,7,255,0,255);
  check(panel.readPixel(4,7) == panel.Magenta, "RGB drawing");
  panel.setTextRgb888(255,255,0); panel.fillScreen(panel.RgbText);
  check(panel.readPixel(95,47) == panel.Yellow, "RGB text sentinel in fill");
}

void testPins() {
  for (int pin = 0; pin < 32; ++pin) {
    auto bad = pins; bad.c = pin;
    Hub75_96x48 panel(bad);
    const unsigned before = calls;
    const bool allowed = ((pin <= 22 || (pin >= 26 && pin <= 28)) &&
                          (pin < 2 || pin > 12));
    check(panel.begin() == allowed, "reject duplicate and unavailable Pico GPIOs");
    if (!allowed) check(calls == before, "invalid pin configuration touches no hardware");
  }
  auto bad = pins; bad.c = 255;
  Hub75_96x48 panel(bad);
  check(!panel.begin(), "large pin index rejected before shifting");
}

void testScan() {
  Hub75_96x48 panel(pins);
  events.clear();
  check(panel.begin(), "valid pin map starts");
  unsigned resets = 0;
  for (const auto& e : events) if (e.pin == pins.a && e.value) {
    ++resets;
    check(e.state & bit(pins.oe), "blanked during startup row reset");
    check((e.state & bit(pins.b)) && !(e.state & bit(pins.c)), "startup shifts zeros with BK high");
  }
  check(resets == 32, "32 startup reset clocks");
  const unsigned before = calls;
  panel.begin(); check(calls == before, "begin is idempotent");
  for (int y = 0; y < 48; ++y) for (int x = 0; x < 96; ++x)
    panel.drawPixel(x, y, (x + y) % 8);
  for (unsigned brightness : {0u,1u,25u,100u,255u}) {
    panel.setBrightness(brightness);
    check(panel.brightness() == std::min(brightness,100u), "brightness clamped");
    events.clear(); const auto start = now;
    panel.refresh();
    check(now - start == 8000, "minimum frame period retained at all brightness levels");
    unsigned pixels = 0, rows = 0, latches = 0, enables = 0;
    uint64_t enabledAt = 0;
    for (const auto& e : events) {
      if (e.pin == pins.clk && e.value) {
        check(e.state & bit(pins.oe), "blanked while shifting RGB");
        const unsigned row = pixels / 96, x = pixels % 96;
        const unsigned packed = panel.framebuffer()[row * 96 + x];
        const unsigned rgb[] = {pins.r1,pins.g1,pins.b1,pins.r2,pins.g2,pins.b2};
        for (unsigned c = 0; c < 6; ++c)
          check(bool(e.state & bit(rgb[c])) == bool(packed & bit(c)), "GPIO output matches framebuffer");
        ++pixels;
      }
      if (e.pin == pins.lat && e.value) {
        ++latches; check(e.state & bit(pins.oe), "blanked while latching");
        check(pixels == latches * 96, "exactly 96 clocks per latch");
      }
      if (e.pin == pins.a && e.value) {
        check(e.state & bit(pins.oe), "blanked while selecting row");
        check(e.state & bit(pins.b), "BK high for row clock");
        check(bool(e.state & bit(pins.c)) == (rows == 0), "one first-row token each frame");
        check(latches == rows + 1, "RGB latched before advancing row");
        ++rows;
      }
      if (e.pin == pins.oe) {
        if (!e.value) { ++enables; enabledAt = e.time; }
        else if (enabledAt) {
          check(e.time - enabledAt == std::min(brightness,100u), "row dwell matches brightness");
          enabledAt = 0;
        }
      }
    }
    check(pixels == 2304 && rows == 24 && latches == 24, "full 24-stage scan including black pixels");
    check(enables == (brightness ? 24u : 0u), "brightness zero never enables output");
    check(state & bit(pins.oe), "panel blanked on return");
  }
}

int main() {
  testFramebuffer(); testPins(); testScan();
  std::cout << "PASS: pixel mapping, rotations, GPIO validation, RGB scan trace, row sequence, OE and timing\n";
}

// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <Hub75_96x48.h>
#include <pico/util/queue.h>
#include <atomic>
#include <stdio.h>
#include <string.h>
#include "PanelConfig.h"

bool core1_separate_stack = true;
Hub75_96x48 panel(panelPins);
queue_t commands;
std::atomic<bool> queueReady{false};
struct Command { char text[96]; };

void grid() {
  panel.clear();
  panel.drawFastVLine(0, 0, 48, panel.Red);
  panel.drawFastVLine(95, 0, 48, panel.Blue);
  panel.drawFastHLine(1, 0, 94, panel.Green);
  panel.drawFastHLine(1, 47, 94, panel.Yellow);
  for (int y = 1; y < 47; ++y) {
    panel.drawPixel(y * 2, y, y < 24 ? panel.Cyan : panel.Magenta);
    for (int b = 0; b < 6; ++b)
      if (y & (1 << b)) panel.fillRect(4 + b * 3, y, 2, 1, panel.White);
  }
}

// All drawing and refresh happens on core 1. Core 0 only queues USB commands.
void apply(const Command& command) {
  const char* s = command.text;
  int x, y, color, value;
  char extra;
  if (!strcmp(s, "grid")) grid();
  else if (!strcmp(s, "blank")) panel.clear();
  else if (!strcmp(s, "text")) {
    panel.clear(); panel.setTextColor(panel.White); panel.setCursor(9, 10);
    panel.print("96 x 48"); panel.setCursor(9, 27); panel.print("SM5368");
  } else if (sscanf(s, "pixel %d %d %d %c", &x, &y, &color, &extra) == 3 &&
             x >= 0 && x < 96 && y >= 0 && y < 48 && color >= 0 && color <= 7) {
    panel.clear(); panel.drawPixel(x, y, color);
  } else if (sscanf(s, "row %d %c", &value, &extra) == 1 && value >= 0 && value < 24) {
    panel.clear(); panel.drawFastHLine(0, value, 96, panel.Red);
    panel.drawFastHLine(0, value + 24, 96, panel.Green);
  } else if (sscanf(s, "solid %d %c", &value, &extra) == 1 && value >= 0 && value <= 7) {
    panel.fillScreen(value);
  } else if (sscanf(s, "brightness %d %c", &value, &extra) == 1 && value >= 0 && value <= 100) {
    panel.setBrightness(value);
  }
}

void setup() {
  queue_init(&commands, sizeof(Command), 4);
  queueReady.store(true, std::memory_order_release);
  Serial.begin(115200);
}

void loop() {
  static Command incoming = {};
  static unsigned length = 0;
  static bool overflow = false;
  // Bounded work; a busy terminal cannot block core 1's refresh.
  for (unsigned n = 0; n < 64 && Serial.available(); ++n) {
    const char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (length && !overflow) {
        incoming.text[length] = '\0';
        if (!strcmp(incoming.text, "help")) {
          Serial.println("grid | text | blank | pixel X Y COLOR | row 0..23");
          Serial.println("solid 0..7 | brightness 0..100 (Enter sends a command)");
          Serial.println("Colors: black=0 red=1 green=2 yellow=3 blue=4 magenta=5 cyan=6 white=7");
        } else {
          Serial.println(queue_try_add(&commands, &incoming) ? "Queued (see help for syntax)" : "Busy; retry");
        }
      }
      length = 0; overflow = false;
    } else if (length + 1 < sizeof(incoming.text)) incoming.text[length++] = c;
    else overflow = true;
  }
  delay(1);
}

void setup1() {
  while (!queueReady.load(std::memory_order_acquire)) delay(1);
  if (!panel.begin()) { while (true) delay(1000); }
  grid();
}

void loop1() {
  Command command;
  // At most one redraw per scan, even when the command queue is full.
  if (queue_try_remove(&commands, &command)) apply(command);
  panel.refresh();
}

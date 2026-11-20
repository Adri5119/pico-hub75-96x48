// SPDX-License-Identifier: MIT
#pragma once
#include <Hub75_96x48.h>

// GENERIC WIRING EXAMPLE, NOT an adapter pinout. GPIO numbers, not header pins.
// Edit every value to match your wiring/adapter before uploading.
// R/G/B mean the actual displayed colors: swap entries if your panel swaps R/B.
constexpr Hub75_96x48::Pins panelPins = {
  2, 3, 4,    // R1, G1, B1
  5, 6, 7,    // R2, G2, B2
  8, 9, 10,   // CLK, LAT, OE
  11, 12, 13  // A (row clock), B (BK), C (row data). D/E unused.
};

// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <array>
#include <cstdint>

namespace cboy::display {

using Frame = std::array<std::array<uint16_t, 160>, 144>;

void draw();
void set_params(uint8_t scanline);
void toggle_fullscreen();

} // namespace cboy::display
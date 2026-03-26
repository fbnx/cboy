// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>

namespace cboy::instructions {

class CPU;

// Execute an instruction based on opcode
uint8_t execute(uint8_t opcode, CPU& cpu);

namespace cb {
    // Execute a CB-prefixed instruction
    uint8_t execute(uint8_t opcode);
} // namespace cb

} // namespace cboy::instructions
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>
#include "display.hpp"

namespace cboy {

struct Frame;

class CPU {
public:
    struct Registers {
        uint8_t A = 0;
        uint8_t F = 0;
        uint8_t B = 0;
        uint8_t C = 0;
        uint8_t D = 0;
        uint8_t E = 0;
        uint8_t H = 0;
        uint8_t L = 0;
    };

    CPU();
    
    // 16-bit register pairs
    uint16_t AF() const { return (registers.A << 8) + registers.F; }
    uint16_t BC() const { return (registers.B << 8) + registers.C; }
    uint16_t DE() const { return (registers.D << 8) + registers.E; }
    uint16_t HL() const { return (registers.H << 8) + registers.L; }
    uint16_t SP() const { return stack_pointer; }
    
    void set_AF(uint16_t value);
    void set_BC(uint16_t value);
    void set_DE(uint16_t value);
    void set_HL(uint16_t value);
    void set_SP(uint16_t value) { stack_pointer = value; }
    
    // Flag operations
    bool get_flag_bit(uint8_t bit) const { return (registers.F >> bit) & 1; }
    void set_flag_bit(uint8_t bit, bool set);
    
    bool flag_Z() const { return get_flag_bit(7); }
    bool flag_N() const { return get_flag_bit(6); }
    bool flag_H() const { return get_flag_bit(5); }
    bool flag_C() const { return get_flag_bit(4); }
    
    void set_flag_Z(bool set) { set_flag_bit(7, set); }
    void set_flag_N(bool set) { set_flag_bit(6, set); }
    void set_flag_H(bool set) { set_flag_bit(5, set); }
    void set_flag_C(bool set) { set_flag_bit(4, set); }
    
    Frame next_frame();
    
    Registers registers;
    uint16_t program_counter = 0x100;
    uint16_t stack_pointer = 0xFFFF;
    bool interrupt_master_enable = true;
    bool halt = false;

private:
    uint8_t fetch();
    uint8_t next_instruction();
    void next_instructions(int cycles);
    void check_interrupt();
};

extern CPU cpu;

} // namespace cboy
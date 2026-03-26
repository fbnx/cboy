// SPDX-License-Identifier: GPL-3.0-only

#include "cpu.hpp"
#include "display.hpp"
#include "gameboy.hpp"
#include "timer.hpp"
#include "instructions/instructions.hpp"
#include "instructions/cb.hpp"
#include <array>

namespace cboy {

CPU cpu;

CPU::CPU() = default;

void CPU::set_AF(uint16_t value) {
    registers.A = (value >> 8) & 0xFF;
    registers.F = value & 0xF0;
}

void CPU::set_BC(uint16_t value) {
    registers.B = (value >> 8) & 0xFF;
    registers.C = value & 0xFF;
}

void CPU::set_DE(uint16_t value) {
    registers.D = (value >> 8) & 0xFF;
    registers.E = value & 0xFF;
}

void CPU::set_HL(uint16_t value) {
    registers.H = (value >> 8) & 0xFF;
    registers.L = value & 0xFF;
}

void CPU::set_flag_bit(uint8_t bit, bool set) {
    if (set) {
        registers.F |= (1 << bit);
    } else {
        registers.F &= ~(1 << bit);
    }
}

uint8_t CPU::fetch() {
    uint8_t value = gameboy::mmu::read(program_counter);
    program_counter += 1;
    return value;
}

void CPU::check_interrupt() {
    uint8_t interrupt_enable = gameboy::mmu::read(0xFFFF);
    uint8_t interrupt_flag = gameboy::mmu::read(0xFF0F);
    
    for (uint8_t i = 0; i < 5; ++i) {
        if ((interrupt_enable >> i & 1) && (interrupt_flag >> i & 1)) {
            if (halt)
                halt = false;
            
            if (!interrupt_master_enable)
                return;
            
            gameboy::mmu::write(0xFF0F, gameboy::mmu::read(0xFF0F) & ~(1 << i));
            interrupt_master_enable = false;
            
            gameboy::mmu::write(stack_pointer - 1, program_counter >> 8);
            gameboy::mmu::write(stack_pointer - 2, program_counter & 0xFF);
            stack_pointer -= 2;
            
            program_counter = 0x40 + i * 8;
            halt = false;
        }
    }
}

uint8_t CPU::next_instruction() {
    check_interrupt();
    
    if (halt)
        return 12;
    
    uint8_t opcode = fetch();
    
    if (opcode == 0xCB) {
        opcode = fetch();
        return instructions::cb::execute(opcode);
    }
    
    return instructions::execute(opcode, *this);
}

void CPU::next_instructions(int cycles) {
    uint8_t cur_cycles;
    while (cycles > 0) {
        cur_cycles = next_instruction();
        timer(cur_cycles);
        cycles -= cur_cycles;
    }
}

Frame CPU::next_frame() {
    while (!gameboy::mmu::lcd_display_enable()) {
        gameboy::mmu::set_mode(0);
        gameboy::mmu::write(0xFF44, 0);
        for (uint8_t i = 0; i < 154; ++i) {
            next_instructions(456);
        }
    }
    
    // Resolution - 160x144 (20x18 tiles)
    // 144 vertical lines
    for (uint8_t i = 0; i < 144; ++i) {
        gameboy::mmu::set_ly(i);
        
        // MODE 2: 77-83 clks
        gameboy::mmu::set_mode(2);
        next_instructions(80);
        
        // MODE 3: 169-175 clks
        gameboy::mmu::set_mode(3);
        next_instructions(172);
        display::set_params(i);
        
        // MODE 0: 201-207 clks
        gameboy::mmu::set_mode(0);
        next_instructions(204);
    }
    
    gameboy::mmu::set_vblank();
    display::draw();
    
    for (uint8_t i = 144; i <= 154; ++i) {
        gameboy::mmu::set_ly(i);
        // MODE 1: 4560 clks
        gameboy::mmu::set_mode(1);
        next_instructions(456);
    }
    
    return gameboy::instance().get_framebuffer();
}

} // namespace cboy
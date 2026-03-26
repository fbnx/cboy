// SPDX-License-Identifier: GPL-3.0-only

#include "gameboy.hpp"
#include "cpu.hpp"
#include <fstream>
#include <cstring>
#include <iostream>

namespace cboy {

Gameboy& Gameboy::instance() {
    static Gameboy gb;
    return gb;
}

void Gameboy::load_rom(const std::string& path) {
    std::cout << "ROM path: " << path << "\n";
    
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error reading rom file!\n";
        throw std::runtime_error("Cannot open ROM file");
    }
    
    std::streamsize file_len = file.tellg();
    file.seekg(0, std::ios::beg);
    
    m_mmu.load_rom(file, file_len);
    
    // Detect CGB mode
    uint8_t cgb_byte = m_mmu.read_rom(0x143);
    m_cgb = (cgb_byte == 0x80 || cgb_byte == 0xC0);
    
    // Initialize CPU and MMU
    cpu.program_counter = 0x100;
    cpu.stack_pointer = 0xFFFE;
    cpu.set_AF(0x11B0);
    cpu.set_BC(0x0013);
    cpu.set_DE(0x00D8);
    cpu.set_HL(0x014D);
    
    m_mmu.initialize();
}

void Gameboy::load_state() {
    // Implementation
}

void Gameboy::save_state() {
    // Implementation
}

uint8_t Gameboy::get_vram_byte(uint16_t addr, bool bank) const {
    return m_mmu.get_vram_byte(addr, bank);
}

uint32_t Gameboy::get_bg_palette(uint8_t index) const {
    return m_mmu.get_bg_palette(index);
}

uint32_t Gameboy::get_sprite_palette(uint8_t index) const {
    return m_mmu.get_sprite_palette(index);
}

void Gameboy::serial_print(char c) {
    std::cout << c;
}

} // namespace cboy
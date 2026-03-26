// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "display.hpp"
#include "mmu.hpp"

namespace cboy {

class Gameboy {
public:
    static Gameboy& instance();
    
    void load_rom(const std::string& path);
    void load_state();
    void save_state();
    
    bool is_cgb() const { return m_cgb; }
    const display::Frame& get_framebuffer() const { return m_framebuffer; }
    
    void set_pixel(uint8_t x, uint8_t y, uint16_t color) {
        m_framebuffer[x][y] = color;
    }
    
    uint8_t get_controls() const { return m_controls; }
    void set_controls(uint8_t value) { m_controls = value; }
    
    // VRAM access
    uint8_t get_vram_byte(uint16_t addr, bool bank) const;
    
    // Palette access
    uint32_t get_bg_palette(uint8_t index) const;
    uint32_t get_sprite_palette(uint8_t index) const;
    
    void serial_print(char c);
    
private:
    Gameboy() = default;
    
    mmu::MMU m_mmu;
    display::Frame m_framebuffer{};
    uint8_t m_controls = 0xFF;
    bool m_cgb = false;
};

} // namespace cboy
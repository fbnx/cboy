// SPDX-License-Identifier: GPL-3.0-only

#include "display.hpp"
#include "cpu.hpp"
#include "gameboy.hpp"
#include "mmu.hpp"
#include <array>
#include <cstring>

namespace cboy::display {

constexpr uint8_t WIDTH = 160;
constexpr uint8_t HEIGHT = 144;

static std::array<std::array<uint16_t, 256>, 256> background;
static std::array<std::array<uint16_t, 256>, 256> window;

static std::array<uint8_t, HEIGHT + 1> scy{};
static std::array<uint8_t, HEIGHT + 1> scx{};
static std::array<uint8_t, HEIGHT + 1> wy{};
static std::array<uint8_t, HEIGHT + 1> wx{};

void set_params(uint8_t i) {
    scy[i] = gameboy::mmu::read(0xFF42);
    scx[i] = gameboy::mmu::read(0xFF43);
    wy[i] = gameboy::mmu::read(0xFF4A);
    wx[i] = gameboy::mmu::read(0xFF4B);
}

static void draw_greyscale(uint8_t x, uint8_t y, uint8_t color) {
    uint16_t color_value;
    switch (color) {
        case 0: color_value = 0xffff; break;
        case 1: color_value = 0x4210; break;
        case 2: color_value = 0x2108; break;
        case 3: color_value = 0x0000; break;
        default: color_value = 0;
    }
    gameboy::instance().set_pixel(x, y, color_value);
}

static void draw_color(uint8_t x, uint8_t y, uint16_t color) {
    gameboy::instance().set_pixel(x, y, color);
}

static void draw_sprite(uint8_t offset_x, uint8_t offset_y, uint16_t tile_offset, uint8_t attr) {
    uint8_t palette_number = attr & 3;
    bool vram_bank = (attr >> 3) & 1;
    bool obp1 = (attr >> 4) & 1;
    bool x_flip = (attr >> 5) & 1;
    bool y_flip = (attr >> 6) & 1;
    
    for (uint8_t y = 0; y < 8; ++y) {
        for (uint8_t x = 0; x < 8; ++x) {
            if (offset_x + x < 8 || offset_y + y < 16 || 
                offset_x + x >= WIDTH + 8 || offset_y + y >= HEIGHT + 16)
                continue;
            
            uint16_t offset = (y_flip ? 7 - y : y) * 2 + tile_offset - 0x8000;
            uint8_t color = gameboy::instance().get_vram_byte(offset, vram_bank);
            color = ((color >> (x_flip ? x : (7 - x))) & 1) |
                   (((gameboy::instance().get_vram_byte(offset + 1, vram_bank) >> 
                      (x_flip ? x : (7 - x))) & 1) << 1);
            
            if (color == 0)
                continue;
            
            if (gameboy::instance().is_cgb()) {
                uint32_t palette = gameboy::instance().get_sprite_palette(palette_number);
                uint16_t color_value = (palette >> (16 * color)) & 0xffff;
                draw_color(offset_y + y - 16, offset_x + x - 8, color_value);
            } else {
                uint8_t palette = gameboy::mmu::read(obp1 ? 0xFF49 : 0xFF48);
                uint8_t greyscale = (palette >> (color * 2)) & 3;
                draw_greyscale(offset_y + y - 16, offset_x + x - 8, greyscale);
            }
        }
    }
}

static void draw_tile(uint8_t offset_x, uint8_t offset_y, bool is_window,
                     std::array<std::array<uint16_t, 256>, 256>& buffer) {
    uint16_t tile_addr = gameboy::mmu::get_tile(offset_x, offset_y, is_window);
    
    bool map_display_select = is_window ? gameboy::mmu::window_tile_map_display_select() :
                                         gameboy::mmu::bg_tile_map_display_select();
    uint16_t map_addr = (map_display_select ? 0x9C00 : 0x9800) + offset_y * 32 + offset_x;
    uint8_t attr = gameboy::instance().get_vram_byte(map_addr - 0x8000, true);
    
    for (uint8_t y = 0; y < 8; ++y) {
        uint16_t offset = y * 2 + tile_addr - 0x8000;
        uint8_t first = gameboy::instance().get_vram_byte(offset, (attr >> 3) & 1);
        uint8_t second = gameboy::instance().get_vram_byte(offset + 1, (attr >> 3) & 1);
        
        for (uint8_t x = 0; x < 8; ++x) {
            uint8_t color = ((first >> (7 - x)) & 1) | (((second >> (7 - x)) & 1) << 1);
            
            if (gameboy::instance().is_cgb()) {
                uint32_t palette = gameboy::instance().get_bg_palette(attr & 7);
                buffer[offset_x * 8 + x][offset_y * 8 + y] = (palette >> (16 * color)) & 0xffff;
            } else {
                uint8_t palette = gameboy::mmu::read(0xFF47);
                buffer[offset_x * 8 + x][offset_y * 8 + y] = (palette >> (color * 2)) & 3;
            }
        }
    }
}

static void render_bg() {
    for (uint8_t y = 0; y < 32; ++y) {
        for (uint8_t x = 0; x < 32; ++x) {
            draw_tile(x, y, false, background);
        }
    }
    
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            if (gameboy::instance().is_cgb()) {
                draw_color(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
            } else {
                draw_greyscale(y, x, background[(x + scx[y]) % 256][(y + scy[y]) % 256]);
            }
        }
    }
}

static void render_window() {
    if (!gameboy::mmu::window_display_enable())
        return;
    
    for (uint8_t y = 0; y < 32; ++y) {
        for (uint8_t x = 0; x < 32; ++x) {
            draw_tile(x, y, true, window);
        }
    }
    
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            if (x + wx[y] >= 7 && x + wx[y] < WIDTH + 7 && y + wy[y] >= 0 && y + wy[y] < HEIGHT) {
                if (gameboy::instance().is_cgb()) {
                    draw_color(y + wy[y], x + wx[y] - 7, window[x][y]);
                } else {
                    draw_greyscale(y + wy[y], x + wx[y] - 7, window[x][y]);
                }
            }
        }
    }
}

static void render_sprites() {
    for (uint8_t i = 0; i < 0xA0; i += 4) {
        uint8_t y = gameboy::mmu::read(0xFE00 + i);
        uint8_t x = gameboy::mmu::read(0xFE00 + i + 1);
        uint8_t tile = gameboy::mmu::read(0xFE00 + i + 2);
        uint8_t attr = gameboy::mmu::read(0xFE00 + i + 3);
        
        if (gameboy::mmu::obj_sprite_size() == 0) {
            // 8x8 sprite
            draw_sprite(x, y, 0x8000 + tile * 16, attr);
        } else {
            // 8x16 sprite
            draw_sprite(x, y, 0x8000 + (tile & 0xFE) * 16, attr);
            draw_sprite(x, y + 8, 0x8000 + (tile | 1) * 16, attr);
        }
    }
}

void draw() {
    render_bg();
    render_window();
    render_sprites();
}

void toggle_fullscreen() {
    // Implementation depends on display backend
}

} // namespace cboy::display
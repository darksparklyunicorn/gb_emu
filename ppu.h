#pragma once
#include "register.h"
#define LCDC 0xff40



class PPU {
private:
    uint8_t videobuf[5760];
    IORegister ioreg[0x81];
    int dots;
    void bitsetRegister(uint16_t addr, bool v, int index);
    //uint16_t getTile();
    void render_scanline();
public:
    void init();
    void tick();
    uint8_t getRegister(uint16_t addr);
    void setRegister(uint16_t addr, uint8_t v);
    uint8_t* video_callback();
};

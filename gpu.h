#pragma once

#define LCDC 0xff40



class GPU {
private:
    uint8_t videobuf[5760];
    uint8_t getRegister(uint16_t addr);
    void setRegister(uint16_t addr, uint8_t v);
    void setRegisterFlag(uint16_t addr, bool v, int index);
    uint16_t getTile();
public:
    void render_scanline();

    uint8_t* video_callback();
};

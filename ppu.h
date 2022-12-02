#pragma once
#include "register.h"
#include <queue>

class Handler;
class PPU;

class Fetcher {
private: 
    PPU& ppu;
    uint16_t y_tileRow, id, tileID, state;
    uint8_t buf[8];
    void clearQ(std::queue<int>&);
    bool ticks;
public:
    std::queue<int> pixelFIFO;
    Fetcher(PPU&);
    void init();
    void tick();
};

class PPU {
private:
    Handler& handler;
    uint8_t videobuf[5760];
    IORegister ioreg[0x81];
    int dots, tcounts, clockfrac, divcounts, state;
    void bitsetRegister(uint16_t addr, bool v, int index);
    //uint16_t getTile();
    void render_scanline();
    Fetcher fetcher;
    friend class Fetcher;
public:
    PPU(Handler&);
    void init();
    void timer_tick();
    void pixel_tick();
    uint8_t getRegister(uint16_t addr);
    void setRegister(uint16_t addr, uint8_t v);
    uint8_t* video_callback();
    uint16_t x;
};
    

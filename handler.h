#pragma once
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <iostream>
#include <stdlib.h>

class Handler {
public:
    bool debug;
    Handler(bool);
    CPU cpu;
    MMU mmu;
    PPU ppu;
    int ticks;
    void getTileMap(uint8_t *);
    inline void tick();
    inline void init(char *);
    inline int frame_callback(uint8_t *); //returns 0 on successful callback, 1 if no frame
};
int Handler::frame_callback(uint8_t * buf) {
    return ppu.video_callback(buf);
}

void Handler::init(char *romPath) {
    mmu.loadROM(romPath);
    cpu.init();
    mmu.init();
    ppu.init();
}

void Handler::tick() {
    ppu.timer_tick();
    ppu.pixel_tick();
    if (++ticks == 4) {
        cpu.tick();
        mmu.tick();
        ticks=0;
    }
} 



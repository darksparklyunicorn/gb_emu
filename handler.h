#pragma once
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <iostream>
#include <stdlib.h>

class Handler {
public:
    Handler();
    CPU cpu;
    MMU mmu;
    PPU ppu;
    int ticks;
    inline void tick();
    inline void init(char *);
    int frame_callback(uint8_t *); //returns 0 on successful callback, 1 if no frame
};

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



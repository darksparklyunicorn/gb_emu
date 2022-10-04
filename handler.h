#pragma once
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

class Handler {
public:
    Handler();
    CPU cpu;
    MMU mmu;
    PPU ppu;
};

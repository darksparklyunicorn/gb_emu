#pragma once
#include "cpu.h"
#include "mmu.h"

class Handler {
public:
    Handler();
    CPU cpu;
    MMU mmu;
};

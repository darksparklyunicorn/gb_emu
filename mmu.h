#pragma once
#include <stdint.h>
#include "register.h"

class Handler;

class MMU {
private:
    uint8_t memory[0x10000];
    Handler& handler;
    int dmaCycles, dmaStart, dots;
    void initDMA(uint8_t val);
public:
    MMU(Handler&);
    void init();
    uint8_t loadWord(uint16_t addr);
    void storeWord(uint16_t addr, uint8_t val);
    void loadROM(char* data);
    void tick();
    IORegister* getReg();
};

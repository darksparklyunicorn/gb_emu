#pragma once
#include "register.h"
#include <stdint.h>

class Handler;

class CPU {
public:
    CPU(Handler& handler);
    void init();
    void tick();
private:
    Register_8b a, b, c, d, e, f, h, l;
    pairRegister af, bc, de, hl;
    Register_16b pc, sp;
    Handler& handler;
    void decode_inst(uint8_t instruction);
};

#include <stdint.h>
#include "register.h"
#include "ppu.h"


void PPU::init() {
    ioreg[0x07].set(0xf8);//tac
    ioreg[0x0f].set(0xe1);
    ioreg[0x40].set(0x91);//lcdc
    ioreg[0x44].set(0x90);//ly
    dots = 0;
    tcounts = 0;
    clockfrac = 1024;
}

void PPU::bitsetRegister(uint16_t addr, bool v, int index) {
    ioreg[addr].bitset(index, v);
}

uint8_t PPU::getRegister(uint16_t addr) {
    return ioreg[addr].get();
}

void PPU::setRegister(uint16_t addr, uint8_t v) {
    ioreg[addr].set(v);
    if (addr == 0x04)
        ioreg[addr].set(0);
    if (addr == 0x07) {
        uint8_t temp = v&0x03;
        switch (temp) {
            case 0: {clockfrac = 1024; break;}
            case 1: {clockfrac = 16; break;}
            case 2: {clockfrac = 64; break;}
            case 3: {clockfrac = 256; break;}
        }
    }
}

void PPU::timer_tick() {
    if (++divcounts >= 256) {
        ioreg[0x04].set(ioreg[0x04].get()+1);
        divcounts = 0;
    }
    if (++tcounts >= clockfrac && ioreg[0x07].bitget(2)) {
        ioreg[0x05].set(ioreg[0x05].get()+1);
        tcounts = 0;
        if (ioreg[0x05].get() == 0) {
            ioreg[0x05].set(ioreg[0x06].get());
            ioreg[0x0f].bitset(2,1);
        }
    }
}

void PPU::pixel_tick() {
    
    /*
    if (!ioreg[0x40].bitget(7))
        return;
    if (++dots >= 456) {
        dots = 0;
        ioreg[0x44].set(ioreg[0x44].get()+1);
        if (ioreg[0x44].get() >= 153)
            ioreg[0x44].set(0);
    }
    */
}

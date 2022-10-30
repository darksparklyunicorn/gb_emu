#include <stdint.h>
#include "register.h"
#include "ppu.h"

void PPU::init() {
    ioreg[0x40].set(0x91);
    ioreg[0x44].set(0x90);
    dots = 0;
}

void PPU::bitsetRegister(uint16_t addr, bool v, int index) {
    ioreg[addr].bitset(index, v);
}

uint8_t PPU::getRegister(uint16_t addr) {
    return ioreg[addr].get();
}

void PPU::setRegister(uint16_t addr, uint8_t v) {
    ioreg[addr].set(v);
}

void PPU::tick() {
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

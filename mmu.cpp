#include "mmu.h"
#include "ppu.h"
#include "register.h"
#include "handler.h"
#include <stdio.h>
#include <iostream>

MMU::MMU(Handler& hand) : memory{}, handler(hand) {
    std::fill(memory, memory+sizeof(memory), 0);
} 

void MMU::init() {
    dmaCycles = 0;
    dots = 0;
    IME = 0;
}

uint8_t MMU::loadWord(uint16_t addr) {
    if (addr >= 0xff00 && addr <= 0xff7f) {
        return handler.ppu.getRegister(addr-0xff00);
    }

    return memory[addr];
}
void MMU::storeWord(uint16_t addr, uint8_t val) {
    if (addr == 0xff46)
        initDMA(val);
    if (addr == 0xFF02 && val == 0x81) {
        std::cout << loadWord(0xFF01);
    }
    if (addr >= 0xff00 && addr <= 0xff7f) {
        handler.ppu.setRegister(addr-0xff00, val);
        return;
    }
    
    memory[addr] = val;

}
void MMU::initDMA(uint8_t val) {
    dmaCycles = 0x9f;
    dmaStart = val<<8;
    dots = 0;
}

void MMU::tick() {
    if (dmaCycles) {
        storeWord( 0xfe9f-dmaCycles, loadWord(dmaStart + 0x9f - dmaCycles));
        dmaCycles--;
    }
    //printf("%x, %x\n", memory[0xff44], handler.ppu.getRegister(0x44));
    dots += 4;
    if (dots >= 456 ) {
        dots = 0;
        if (memory[0xff44]++ >= 153)
            memory[0xff44] = 0;
    }
}

void MMU::loadROM(char* str) {
    FILE* fp = fopen(str, "rb");
    uint8_t* p = (uint8_t*)memory;
    while (fread(p++, sizeof(uint8_t), 1,fp));
    
    fclose(fp);  


}


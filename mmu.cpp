#include "mmu.h"
#include "register.h"
#include "handler.h"
#include <stdio.h>
#include <iostream>

MMU::MMU(Handler& hand) : handler(hand) {} 

void MMU::init() {
    dmaCycles = 0;
}

uint8_t MMU::loadWord(uint16_t addr) {
    if (addr >= 0xff00 && addr <= 0xff70) {
        return handler.ppu.ioreg[addr-0xff00].get();
    }

    return memory[addr];
}
void MMU::storeWord(uint16_t addr, uint8_t val) {
    memory[addr] = val;
    if (addr == 0xff46)
        initDMA(val);
    if (addr == 0xFF02 && val == 0x81) {
        std::cout << loadWord(0xFF01) << std::endl;
    }
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


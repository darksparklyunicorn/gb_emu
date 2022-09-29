#include "mmu.h"
#include <stdio.h>

void MMU::init() {
    dmaCycles = 0;
}

uint8_t MMU::loadWord(uint16_t addr) {
    return memory[addr/4];
}
void MMU::storeWord(uint16_t addr, uint8_t val) {
    memory[addr/4] = val;
    if (addr == 0xff46)
        initDMA(val);
}
void MMU::initDMA(uint8_t val) {
    dmaCycles = 0x9f;
    dmaStart = val<<8;
}

void MMU::tick() {
    if (dmaCycles) {
        storeWord( 0xfe9f-dmaCycles, loadWord(dmaStart + 0x9f - dmaCycles));
        dmaCycles--;
    }
}

void MMU::loadROM(char* str) {
    FILE* fp = fopen(str, "rb");
    uint8_t* p = (uint8_t*)memory;
    while (fread(p++, sizeof(uint8_t), 1,fp));
    fclose(fp);    
}


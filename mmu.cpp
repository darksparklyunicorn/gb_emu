#include "mmu.h"
#include <stdio.h>

uint8_t MMU::loadWord(uint16_t addr) {
    return memory[addr/4];
}
void MMU::storeWord(uint16_t addr, uint8_t val) {
    memory[addr/4] = val;
}
void MMU::loadROM(char* str) {
    FILE* fp = fopen(str, "rb");
    uint8_t* p = (uint8_t*)memory;
    while (fread(p, sizeof(uint8_t), 1,fp));
    fclose(fp);    
}


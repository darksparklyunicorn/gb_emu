#pragma once

class MMU {
private:
    uint8_t memory[0x2222];
public:
    void init();
    auto loadWord(uint16_t addr);
    void storeWord(uint16_t addr, uint8_t val);
    void loadROM();
};

#pragma once
#include <stdint.h>

class Register_8b {
private:
    uint8_t value = 0x0;
public:
    void set(uint8_t v);
    auto get();
};
class pairRegister {
private:
    Register_8b& u_val, l_val;
public:
    pairRegister(Register_8b& u, Register_8b& l); 
    void set(uint16_t v);
    auto get();
};
class Register_16b {
private:
    uint16_t value = 0x0;
public:
    void set(uint16_t v);
    auto get();
    void add(uint16_t v);
};


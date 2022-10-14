#pragma once
#include <stdint.h>

class Register_8b {
protected:
    uint8_t value;
public:
    Register_8b() : value(0) {}
    void set(uint8_t v);
    uint8_t get();
};

class IORegister : public Register_8b {
public:
    IORegister();
    void bitset(int index, bool value);
    bool bitget(int index);
};

class flagRegister : public Register_8b {
public:
    void setZ(bool b);
    void setN(bool b);
    void setH(bool b);
    void setC(bool b);
    bool getZ();
    bool getN();
    bool getH();
    bool getC();
};

class pairRegister {
private:
    Register_8b &u_val, &l_val;
public:
    pairRegister(Register_8b& u, Register_8b& l); 
    void set(uint16_t v);
    uint16_t get();
};

class Register_16b {
protected:
    uint16_t value = 0x0;
public:
    void set(uint16_t v);
    uint16_t get();
};


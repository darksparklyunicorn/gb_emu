#include "register.h"

//8 bit register
void Register_8b::set(uint8_t val) {
    value = val;
}
uint8_t Register_8b::get() {
    return value;
}
//IORegister
IORegister::IORegister() {
value = 0;
}
void IORegister::bitset(int index, bool val) {
    value = (value & ~(1<<index)) | (val<<index);
}
bool IORegister::bitget(int index) {
    return ((value>>index) & 0x01); 
}

//flag register
void flagRegister::setZ(bool b) {
    value = (value & 0x7f) | (b<<7);
}
void flagRegister::setN(bool b) {
    value = (value & 0xbf) | (b<<6);
}
void flagRegister::setH(bool b) {
    value = (value & 0xdf) | (b<<5);
}
void flagRegister::setC(bool b) {
    value = (value & 0xef) | (b<<4);
}

bool flagRegister::getZ() {
    return (value >> 7) & 1;
}
bool flagRegister::getN() {
    return (value >> 6) & 1;
}
bool flagRegister::getH() {
    return (value >> 5) & 1;
}
bool flagRegister::getC() {
    return (value >> 4) & 1;
}

//pair register
pairRegister::pairRegister(Register_8b& u, Register_8b& l) : u_val(u), l_val(l) {
}
uint16_t pairRegister::get() {
    return (u_val.get() << 8) | l_val.get();
}
void pairRegister::set(uint16_t val) {
    u_val.set((val&0xff00) >> 8);
    l_val.set(val&0x00ff);
}
   

//16 bit register
void Register_16b::set(uint16_t val) {
    value = val;
}
uint16_t Register_16b::get() {
    return value;
}


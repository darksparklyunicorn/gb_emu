#include "register.h"

void Register_8b::set(uint8_t val) {
    value = val;
}
uint8_t Register_8b::get() {
    return value;
}
pairRegister::pairRegister(Register_8b& u, Register_8b& l) : u_val(u), l_val(l) {
}
void Register_16b::set(uint16_t val) {
    value = val;
}
uint16_t Register_16b::get() {
    return value;
}
void Register_16b::add(uint16_t val) {
    value += val;
}

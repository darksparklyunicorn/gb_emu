includes "register.h"

void 8bRegister::set(uint8_t val) {
    value = val;
}
auto 8bRegister::get() {
    return value;
}
void 16bRegister::set(uint16_t val) {
    value = val;
}
auto 16bRegister::get() {
    return value;
}
void 16bRegister::add(uint16_t val) {
    value += val;
}

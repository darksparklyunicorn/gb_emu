#include "cpu.h"
#include "mmu.h"
#include <iostream>
#include "register.h"


CPU::CPU(Handler & hand) : handler(hand), af(a,f), bc(b,c), de(d,e), hl(h,l) {
}

void CPU::init() {
   pc.set(0x100);
}
void CPU::tick() {
    decode_inst(handler.mmu.loadWord(pc.get()));
    pc.add(4);
}    

void CPU::decode_inst(uint8_t instruction) {
    switch (instruction) {
        case 0: {cout<<a.get(); break;}
        case 1: {cout<<b.get(); break;}
        case 2: {a.set(0x12); break;}
        case 3: {handler.mmu.storeWord(0x0000, a.get()); break;}
        case 4: {b.set(handler.mmu.loadWord(0x0000)); break;}
    }
}

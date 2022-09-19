#include "cpu.h"
#include "mmu.h"
#include <iostream>
#include "register.h"
#include "handler.h"

CPU::CPU(Handler& hand) : handler(hand), af(a,f), bc(b,c), de(d,e), hl(h,l) {
}

void CPU::init() {
   pc.set(0x0100);
}
void CPU::tick() {
    auto temp2 = pc.get();
    uint8_t temp = handler.mmu.loadWord(temp2);
    decode_inst(temp);
    pc.add(4);
}    

void CPU::decode_inst(uint8_t instruction) {
    switch (instruction) {
        case 0: {std::cout<<a.get(); break;}
        case 1: {std::cout<<b.get(); break;}
        case 2: {a.set(0x12); break;}
        case 3: {handler.mmu.storeWord(0x0000, a.get()); break;}
        case 4: {auto temp = handler.mmu.loadWord(0x0000); b.set(temp); break;}
    }
}

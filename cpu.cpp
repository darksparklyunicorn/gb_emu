#include "cpu.h"
#include "mmu.h"
#include <iostream>
#include "register.h"
#include "handler.h"

CPU::CPU(Handler& hand) : handler(hand), af(a,f), bc(b,c), de(d,e), hl(h,l) {
}

void CPU::debug2() {
    Register_8b& temp = f;
    printf("%x %x\n", temp.get(), f.get());
    f.set(3);
    printf("%x %x\n", temp.get(), f.get());
    temp.set(0xa);
    printf("%x %x\n", temp.get(), f.get());
}

void CPU::debug() {
    fprintf(stderr,"af= %04x bc= %04x\nde= %04x hl= %04x\nsp= %04x pc= %04x\nf:%d%d%d%d\n\n", af.get(), bc.get(), de.get(), hl.get(), sp.get(), pc.get(), f.getZ(), f.getN(), f.getH(), f.getC()); 
    /* << "af= " << af.get() << " bc= " << bc.get() << std::endl;
    std::cout << "de= " << de.get() << " hl= " << hl.get() << std::endl;
    std::cout << "sp= " << sp.get() << " pc= " << pc.get() << std::endl;
    std::cout << "f:" << f.getZ() << f.getN() << f.getH() << f.getC() << std::endl << std::endl;*/
}
void CPU::init() {
    cycles=0;
    pc.set(0x0100);
    sp.set(0xfffe);
    af.set(0x01b0);
    bc.set(0x0013);
    de.set(0x00d8);
    hl.set(0x014d);
}
void CPU::tick() {
    if (cycles > 0) {
        cycles--;
        return;
    }
    debug();
    uint8_t temp = fetchPC();
    decode_inst(temp);
    //printf("%d \n", temp);
}    
//instructions
uint8_t CPU::fetchPC() {
    uint8_t data = handler.mmu.loadWord(pc.get());
    pc.set(pc.get()+1);
    //printf("inst: %d \n", data);
    return data;
}

void CPU::NOP() {
    cycles = 0;
}

//load immediate
void CPU::loadimm(Register_8b& r) {
    uint8_t temp = fetchPC();
    r.set(temp);
    cycles = 1;
}

void CPU::loadimm(Register_16b& r) {
    uint16_t temp = fetchPC() | fetchPC() << 8;
    r.set(temp);
    cycles = 2;
}

void CPU::loadimm(pairRegister& r) {
    uint16_t temp = fetchPC() | fetchPC() << 8;
    r.set(temp);
    cycles = 2;
}

//inc
void CPU::inc(pairRegister& r) {
    r.set(r.get()+1);
    cycles = 1;
}

void CPU::inc(Register_16b& r) {
    r.set(r.get()+1);
    cycles = 1;
}

void CPU::inc(Register_8b& r) {
    uint8_t temp = r.get();
    r.set(temp+1);
    f.setH((((temp&0x0f)+(0x01))&0x10) == 0x10);
    f.setN(false);
    f.setZ(((temp+1)&0xff)==0);
}

void CPU::inc_HL() {//increment word at memory address stored in HL
    uint16_t temp_addr = hl.get();
    uint8_t temp = handler.mmu.loadWord(temp_addr);
    f.setH((((temp++&0x0f)+(0x01))&0x10) == 0x10);
    f.setN(false);
    f.setZ(temp==0); 
    handler.mmu.storeWord(temp_addr, temp);   
    cycles = 2;
}

//dec
void CPU::dec(pairRegister& r) {
    r.set(r.get()-1);
    cycles = 1;
}

void CPU::dec(Register_16b& r) {
    r.set(r.get()-1);
    cycles = 1;
}

void CPU::dec(Register_8b& r) {
    uint8_t temp = r.get();
    r.set(temp-1);
    f.setH((((temp&0x0f)-(0x01))&0x10) == 0x10);
    f.setN(true);
    f.setZ(temp-1==0);
}

void CPU::dec_HL() {//decrement word at memory address stored in HL
    uint16_t temp_addr = hl.get();
    uint8_t temp = handler.mmu.loadWord(temp_addr);
    f.setH((((temp--&0x0f)-(0x01))&0x10) == 0x10);
    f.setN(true);
    f.setZ(temp==0); 
    handler.mmu.storeWord(temp_addr, temp);   
    cycles = 2;
}

//barrel shift 
void CPU::rlca() { //barrel shift register a 1 left
    uint8_t temp = a.get();
    auto carry = temp & 0x80;
    temp = temp << 1;
    a.set( (temp&0xfe) | !!carry);
    f.setC(carry);
    f.setZ(false);
    f.setN(false);
    f.setH(false);
}
void CPU::rla() {
    auto temp = a.get();
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x80);
    temp = temp << 1;
    a.set((temp&0xfe) | flagbit);
    f.setZ(false);
    f.setN(false);
    f.setH(false);
}
void CPU::rrca() { //barrel shift register a 1 right
    uint8_t temp = a.get();
    auto carry = temp & 0x01;
    temp = temp >> 1;
    a.set(carry << 7  | (temp&0x7f));
    f.setC(carry);
    f.setZ(false);
    f.setN(false);
    f.setH(false);
}
void CPU::rra() {
    auto temp = a.get();
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x01);
    temp = temp >> 1;
    a.set(flagbit << 7  | (temp&0x7f));
    f.setZ(false);
    f.setN(false);
    f.setH(false);
}


//store to memory
void CPU::storeWord(uint16_t addr, uint8_t data) {
    handler.mmu.storeWord(addr, data);
    cycles = 1;
}

void CPU::sw_sp() { //store sp at 16b address in instruction
    uint16_t addr = fetchPC() | fetchPC()<<8; //little endian format
    handler.mmu.storeWord(addr, (uint8_t)(sp.get() & 0x00ff));
    handler.mmu.storeWord(addr+1, (uint8_t)((sp.get()&0xff00) >> 8));
    cycles = 2;
}

void CPU::sw_HL_imm() {
    uint8_t imm = fetchPC();
    handler.mmu.storeWord(hl.get(), imm);
    cycles = 2;
}

void CPU::sw_A_imm16() {
    uint16_t addr = fetchPC() | fetchPC()<<8;
    handler.mmu.storeWord(addr, a.get());
    cycles = 3;
}

void CPU::sw_A_imm(uint8_t imm) {
    uint16_t addr = imm | 0xff00;
    handler.mmu.storeWord(addr, a.get());
    cycles = 2;
}


//load from memory
void CPU::loadWord(uint16_t addr, Register_8b& r) {
    r.set(handler.mmu.loadWord(addr));
    cycles = 1;
}

uint8_t CPU::fetchWord(uint16_t addr) {
    cycles = 1;
    return handler.mmu.loadWord(addr);
}

void CPU::lw_A_imm(uint8_t imm) {
    uint16_t temp = imm | 0xff00;
    a.set(fetchWord(temp));
    cycles = 2;
}

void CPU::lw_A_imm16() {
    uint16_t temp = fetchPC() | fetchPC()<<8;
    a.set(fetchWord(temp));
    cycles = 3;
}

void CPU::lw_hl_sp() {
    uint8_t temp = fetchPC();
    int imm = (temp&0x7f) - (temp&0x80);
    uint16_t temp2 = sp.get();
    f.setH((((temp2&0xff)+(imm&0xff)) & 0x0100));
    f.setN(false);
    uint32_t carry = (temp2 + imm) & 0x00010000;
    f.setC(carry);
    hl.set(temp2+imm);
    cycles = 2;
}

   
//add
void CPU::add_HL(pairRegister& r) {
    uint16_t HL_val = hl.get();
    uint16_t r_val = r.get();
    f.setH((((HL_val&0xff)+(r_val&0xff)) & 0x0100));
    f.setN(false);
    uint32_t carry = (HL_val + r_val) & 0x00010000;
    f.setC(carry);
    hl.set(HL_val+r_val);
    cycles = 1;
}

void CPU::add_HLSP() {
    uint16_t HL_val = hl.get();
    uint16_t SP_val = sp.get();
    f.setH((((HL_val&0xff)+(SP_val&0xff)) & 0x0100));
    f.setN(false);
    uint32_t carry = (HL_val + SP_val) & 0x00010000;
    f.setC(carry);
    hl.set(HL_val+SP_val);
    cycles = 1;
}

void CPU::add(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setH((((r1_val&0x0f)+(r2_val&0x0f))&0x10) == 0x10);
    uint16_t carry = (r1_val + r2_val)&0x0100; 
    f.setN(false);
    f.setZ(((r1_val + r2_val)&0xff) == 0);
    f.setC(carry);
    r1.set(r1_val + r2_val);
}

void CPU::add_SP_imm() {
    uint8_t temp = fetchPC();
    int imm = (temp&0x7f) - (temp&0x80);
    uint16_t temp2 = sp.get();
    f.setH((((temp2&0xff)+(imm&0xff)) & 0x0100));
    f.setN(false);
    uint32_t carry = (temp2 + imm) & 0x00010000;
    f.setC(carry);
    sp.set(temp2+imm);
    cycles = 3;
}

void CPU::adc(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    bool carryIn = f.getC();
    f.setH((((r1_val&0x0f)+(r2_val&0x0f)+carryIn)&0x10) == 0x10);
    uint16_t carry = (r1_val + r2_val + carryIn)&0x0100; 
    f.setN(false);
    f.setZ(((r1_val + r2_val + carryIn)&0xff) == 0);
    f.setC(carry);
    r1.set(r1_val + r2_val + carryIn);
}

//sub
void CPU::sub(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setH((((r1_val&0x0f)-(r2_val&0x0f))&0x10) == 0x10);
    uint16_t carry = (r1_val - r2_val)&0x0100; 
    f.setN(true);
    f.setZ(r1_val - r2_val == 0);
    f.setC(carry);
    r1.set(r1_val - r2_val);
}

void CPU::sbc(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    bool carryIn = f.getC();
    f.setH((((r1_val&0x0f)-(r2_val&0x0f)+carryIn)&0x10) == 0x10);
    uint16_t carry = (r1_val - r2_val - carryIn)&0x0100; 
    f.setN(false);
    f.setZ(r1_val - r2_val - carryIn == 0);
    f.setC(carry);
    r1.set(r1_val - r2_val - carryIn);
}

//jump
void CPU::jr() {
    uint8_t temp = fetchPC();
    pc.set(pc.get() + (temp&0x7f) - (temp&0x80));
    cycles = 2;
}

void CPU::jr(bool cond) {
    uint8_t temp = fetchPC();
    cycles = 1;
    if (cond) {
        pc.set(pc.get() + (temp&0x7f) - (temp&0x80));
        return;
    }
    cycles++;
}

void CPU::jp() {
    uint16_t temp = fetchPC() | fetchPC()<<8;
    pc.set(temp);
    cycles = 3;
}

void CPU::jp_HL() {
    pc.set(hl.get());
}

void CPU::jp(bool cond) {
    if (cond) 
        jp();
    else 
        cycles = 2;
}

void CPU::call(bool cond) {
    cycles = 2;
    if (cond) {
        uint16_t temp = fetchPC() | fetchPC() << 8;
        push_PC();
        pc.set(temp);
        cycles += 3;
    }
}

void CPU::rst(uint8_t r) {
    uint16_t addr = fetchWord(1<<r);
    push_PC();
    pc.set(addr);
    cycles = 3;    
}

//return
void CPU::pop(pairRegister& r) {
    uint16_t temp = handler.mmu.loadWord(sp.get());
    sp.set(sp.get()+1);
    temp |= handler.mmu.loadWord(sp.get())<<8;
    sp.set(sp.get()+1);
    r.set(temp);
    cycles = 2;
}

void CPU::push(pairRegister& r) {
    uint16_t temp = r.get();
    uint16_t addr = sp.get();
    storeWord(addr-1, (uint8_t)(temp>>8));
    storeWord(addr-2, (uint8_t)(temp&0xff));
    sp.set(addr-2);
    cycles = 3;
}    

void CPU::push_PC() {
    uint16_t temp = pc.get();
    uint16_t addr = sp.get();
    storeWord(addr-1, (uint8_t)(temp>>8));
    storeWord(addr-2, (uint8_t)(temp&0xff));
    sp.set(addr-2);
    cycles = 3;
}    


void CPU::pop_PC() {
    uint16_t temp = handler.mmu.loadWord(sp.get());
    sp.set(sp.get()+1);
    temp |= handler.mmu.loadWord(sp.get())<<8;
    sp.set(sp.get()+1);
    sp.set(temp);
    cycles = 2;
}

void CPU::ret() {
    pop_PC();
    cycles++;
}

void CPU::ret(bool cond) {
    cycles = 1;
    if (cond) {
        pop_PC();
        cycles = 4;
    }
}

//bitwise
void CPU::cpl() {
    a.set(~a.get());
    f.setN(true);
    f.setH(true);
}

void CPU::scf() {
    f.setN(false);
    f.setH(false);
    f.setC(true);
}

void CPU::ccf() {
    f.setN(false);
    f.setH(false);
    f.setC(!f.getC());
}

void CPU::bitwise_and(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ(r1_val & r2_val);
    f.setN(false);
    f.setH(true);
    f.setC(false);
    r1.set(r1_val + r2_val);
}

void CPU::bitwise_xor(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ(r1_val ^ r2_val);
    f.setN(false);
    f.setH(false);
    f.setC(false);
    r1.set(r1_val ^ r2_val);
}

void CPU::bitwise_or(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ(r1_val | r2_val);
    f.setN(false);
    f.setH(false);
    f.setC(false);
    r1.set(r1_val | r2_val);
}

void CPU::bitwise_cp(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ((r1_val - r2_val) == 0);
    f.setH((((r1_val&0x0f)-(r2_val&0x0f))&0x10) == 0x10);
    uint16_t carry = (r1_val - r2_val)&0x0100;
    f.setN(true);
    f.setC(carry);
    r1.set(r1_val + r2_val);
}

//immediate
void CPU::addi() {
    add(a, fetchPC());
    cycles = 1;
}

void CPU::subi() {
    sub(a, fetchPC());
    cycles = 1;
}

void CPU::adci() {
    adc(a, fetchPC());
    cycles = 1;
}

void CPU::sbci() {
    sbc(a, fetchPC());
    cycles = 1;
}

void CPU::andi() {
    bitwise_and(a, fetchPC());
    cycles = 1;
}

void CPU::ori() {
    bitwise_or(a, fetchPC());
    cycles = 1;
}

void CPU::xori() {
    bitwise_xor(a, fetchPC());
    cycles = 1;
}

void CPU::cpi() {
    bitwise_cp(a, fetchPC());
    cycles = 1;
}




void CPU::decode_inst(uint8_t instruction) {
    switch (instruction) {
        case 0x00: {NOP(); break;}
        case 0x01: {loadimm(bc);  break;}
        case 0x02: {storeWord(bc.get(), a.get()); break;}
        case 0x03: {inc(bc); break;}
        case 0x04: {inc(b); break;}
        case 0x05: {dec(b); break;}
        case 0x06: {loadimm(b); break;}
        case 0x07: {rlca(); break;}
        
        case 0x08: {sw_sp(); break;}
        case 0x09: {add_HL(bc); break;}
        case 0x0a: {loadWord(bc.get(), a); break;}
        case 0x0b: {dec(bc); break;}
        case 0x0c: {inc(c); break;}
        case 0x0d: {dec(c); break;}
        case 0x0e: {loadimm(c); break;}
        case 0x0f: {rrca(); break;}

        case 0x10: {break;} //TODO: stop
        case 0x11: {loadimm(de); break;}
        case 0x12: {storeWord(de.get(), a.get()); break;}
        case 0x13: {inc(de); break;}
        case 0x14: {inc(d); break;}
        case 0x15: {dec(d); break;}
        case 0x16: {loadimm(d); break;}
        case 0x17: {rla(); break;}
        
        case 0x18: {jr(); break;}
        case 0x19: {add_HL(de); break;}
        case 0x1a: {loadWord(de.get(), a); break;}
        case 0x1b: {dec(de); break;}
        case 0x1c: {inc(e); break;}
        case 0x1d: {dec(e); break;}
        case 0x1e: {loadimm(e); break;}
        case 0x1f: {rra(); break;}

        case 0x20: {jr(f.getZ()==0); break;}
        case 0x21: {loadimm(hl); break;}
        case 0x22: {storeWord(hl.get(), a.get()); hl.set(hl.get()+1); break;}
        case 0x23: {inc(hl); break;}
        case 0x24: {inc(h); break;}
        case 0x25: {dec(h); break;}
        case 0x26: {loadimm(h); break;}
        case 0x27: { break; } //TODO: DAA

        case 0x28: {jr(f.getZ()==1); break;}
        case 0x29: {add_HL(hl); break; }
        case 0x2a: {loadWord(hl.get(), a); hl.set(hl.get()+1); break;}
        case 0x2b: {dec(hl); break;}
        case 0x2c: {inc(l); break;}
        case 0x2d: {dec(l); break;}
        case 0x2e: {loadimm(l); break;}
        case 0x2f: {cpl(); break;}

        case 0x30: {jr(f.getC()==0); break;}
        case 0x31: {loadimm(sp); break;}
        case 0x32: {storeWord(hl.get(), a.get()); hl.set(hl.get()-1); break;}
        case 0x33: {inc(sp); break;}
        case 0x34: {inc_HL(); break;}
        case 0x35: {dec_HL(); break;}
        case 0x36: {sw_HL_imm(); break;}
        case 0x37: {scf(); break;}
        
        case 0x38: {jr(f.getC()==1); break;}
        case 0x39: {add_HLSP(); break;}
        case 0x3a: {loadWord(hl.get(), a); hl.set(hl.get()-1); break;}
        case 0x3b: {dec(sp); break;}
        case 0x3c: {inc(a); break;}
        case 0x3d: {dec(a); break;}
        case 0x3e: {loadimm(a); break;}
        case 0x3f: {ccf(); break;}
        
        //0x40
        case 0x40: {b.set(b.get()); break;}
        case 0x41: {b.set(c.get()); break;}
        case 0x42: {b.set(d.get()); break;}
        case 0x43: {b.set(e.get()); break;}
        case 0x44: {b.set(h.get()); break;}
        case 0x45: {b.set(l.get()); break;}
        case 0x46: {loadWord(hl.get(), b); break;}
        case 0x47: {b.set(a.get()); break;}

        case 0x48: {c.set(b.get()); break;}
        case 0x49: {c.set(c.get()); break;}
        case 0x4a: {c.set(d.get()); break;}
        case 0x4b: {c.set(e.get()); break;}
        case 0x4c: {c.set(h.get()); break;}
        case 0x4d: {c.set(l.get()); break;}
        case 0x4e: {loadWord(hl.get(), c); break;}
        case 0x4f: {c.set(a.get()); break;}

        case 0x50: {d.set(b.get()); break;}
        case 0x51: {d.set(c.get()); break;}
        case 0x52: {d.set(d.get()); break;}
        case 0x53: {d.set(e.get()); break;}
        case 0x54: {d.set(h.get()); break;}
        case 0x55: {d.set(l.get()); break;}
        case 0x56: {loadWord(hl.get(), d); break;}
        case 0x57: {d.set(a.get()); break;}

        case 0x58: {e.set(b.get()); break;}
        case 0x59: {e.set(c.get()); break;}
        case 0x5a: {e.set(d.get()); break;}
        case 0x5b: {e.set(e.get()); break;}
        case 0x5c: {e.set(h.get()); break;}
        case 0x5d: {e.set(l.get()); break;}
        case 0x5e: {loadWord(hl.get(), e); break;}
        case 0x5f: {e.set(a.get()); break;}

        case 0x60: {h.set(b.get()); break;}
        case 0x61: {h.set(c.get()); break;}
        case 0x62: {h.set(d.get()); break;}
        case 0x63: {h.set(e.get()); break;}
        case 0x64: {h.set(h.get()); break;}
        case 0x65: {h.set(l.get()); break;}
        case 0x66: {loadWord(hl.get(), h); break;}
        case 0x67: {h.set(a.get()); break;}

        case 0x68: {l.set(b.get()); break;}
        case 0x69: {l.set(c.get()); break;}
        case 0x6a: {l.set(d.get()); break;}
        case 0x6b: {l.set(e.get()); break;}
        case 0x6c: {l.set(h.get()); break;}
        case 0x6d: {l.set(l.get()); break;}
        case 0x6e: {loadWord(hl.get(), l); break;}
        case 0x6f: {l.set(a.get()); break;}

        case 0x70: {storeWord(hl.get(), b.get()); break;}
        case 0x71: {storeWord(hl.get(), c.get()); break;}
        case 0x72: {storeWord(hl.get(), d.get()); break;}
        case 0x73: {storeWord(hl.get(), e.get()); break;}
        case 0x74: {storeWord(hl.get(), h.get()); break;}
        case 0x75: {storeWord(hl.get(), l.get()); break;}
        case 0x76: { break;} //TODO HALT
        case 0x77: {storeWord(hl.get(), a.get()); break;}
        
        case 0x78: {a.set(b.get()); break;}
        case 0x79: {a.set(c.get()); break;}
        case 0x7a: {a.set(d.get()); break;}
        case 0x7b: {a.set(e.get()); break;}
        case 0x7c: {a.set(h.get()); break;}
        case 0x7d: {a.set(l.get()); break;}
        case 0x7e: {loadWord(hl.get(), a); break;}
        case 0x7f: {a.set(a.get()); break;}

        //0x80
        case 0x80: {add(a,b.get()); break;}
        case 0x81: {add(a,c.get()); break;}
        case 0x82: {add(a,d.get()); break;}
        case 0x83: {add(a,e.get()); break;}
        case 0x84: {add(a,h.get()); break;}
        case 0x85: {add(a,l.get()); break;}
        case 0x86: {add(a, fetchWord(hl.get())); break;}
        case 0x87: {add(a,a.get()); break;}

        case 0x88: {adc(a,b.get()); break;}
        case 0x89: {adc(a,c.get()); break;}
        case 0x8a: {adc(a,d.get()); break;}
        case 0x8b: {adc(a,e.get()); break;}
        case 0x8c: {adc(a,h.get()); break;}
        case 0x8d: {adc(a,l.get()); break;}
        case 0x8e: {adc(a, fetchWord(hl.get())); break;}
        case 0x8f: {adc(a,a.get()); break;}

        case 0x90: {sub(a,b.get()); break;}
        case 0x91: {sub(a,c.get()); break;}
        case 0x92: {sub(a,d.get()); break;}
        case 0x93: {sub(a,e.get()); break;}
        case 0x94: {sub(a,h.get()); break;}
        case 0x95: {sub(a,l.get()); break;}
        case 0x96: {sub(a, fetchWord(hl.get())); break;}
        case 0x97: {sub(a,a.get()); break;}

        case 0x98: {sbc(a,b.get()); break;}
        case 0x99: {sbc(a,c.get()); break;}
        case 0x9a: {sbc(a,d.get()); break;}
        case 0x9b: {sbc(a,e.get()); break;}
        case 0x9c: {sbc(a,h.get()); break;}
        case 0x9d: {sbc(a,l.get()); break;}
        case 0x9e: {sbc(a, fetchWord(hl.get())); break;}
        case 0x9f: {sbc(a,a.get()); break;}

        case 0xa0: {bitwise_and(a,b.get()); break;}
        case 0xa1: {bitwise_and(a,c.get()); break;}
        case 0xa2: {bitwise_and(a,d.get()); break;}
        case 0xa3: {bitwise_and(a,e.get()); break;}
        case 0xa4: {bitwise_and(a,h.get()); break;}
        case 0xa5: {bitwise_and(a,l.get()); break;}
        case 0xa6: {bitwise_and(a, fetchWord(hl.get())); break;}
        case 0xa7: {bitwise_and(a,a.get()); break;}

        case 0xa8: {bitwise_xor(a,b.get()); break;}
        case 0xa9: {bitwise_xor(a,c.get()); break;}
        case 0xaa: {bitwise_xor(a,d.get()); break;}
        case 0xab: {bitwise_xor(a,e.get()); break;}
        case 0xac: {bitwise_xor(a,h.get()); break;}
        case 0xad: {bitwise_xor(a,l.get()); break;}
        case 0xae: {bitwise_xor(a, fetchWord(hl.get())); break;}
        case 0xaf: {bitwise_xor(a,a.get()); break;}

        case 0xb0: {bitwise_or(a,b.get()); break;}
        case 0xb1: {bitwise_or(a,c.get()); break;}
        case 0xb2: {bitwise_or(a,d.get()); break;}
        case 0xb3: {bitwise_or(a,e.get()); break;}
        case 0xb4: {bitwise_or(a,h.get()); break;}
        case 0xb5: {bitwise_or(a,l.get()); break;}
        case 0xb6: {bitwise_or(a, fetchWord(hl.get())); break;}
        case 0xb7: {bitwise_or(a,a.get()); break;}

        case 0xb8: {bitwise_cp(a,b.get()); break;}
        case 0xb9: {bitwise_cp(a,c.get()); break;}
        case 0xba: {bitwise_cp(a,d.get()); break;}
        case 0xbb: {bitwise_cp(a,e.get()); break;}
        case 0xbc: {bitwise_cp(a,h.get()); break;}
        case 0xbd: {bitwise_cp(a,l.get()); break;}
        case 0xbe: {bitwise_cp(a, fetchWord(hl.get())); break;}
        case 0xbf: {bitwise_cp(a,a.get()); break;}

        //0xc0
        case 0xc0: {ret(f.getZ()==0); break;}
        case 0xc1: {pop(bc); break;}
        case 0xc2: {jp(f.getZ()==0); break;}
        case 0xc3: {jp(); break;}
        case 0xc4: {call(f.getZ()==0); break;} //TODO call
        case 0xc5: {push(bc); break;}
        case 0xc6: {addi(); break;}
        case 0xc7: {rst(0); break;}
        
        case 0xc8: {ret(f.getZ()==1); break;}
        case 0xc9: {ret(); break;}
        case 0xca: {jp(f.getZ()==1); break;}
        case 0xcb: {decode_cb_inst(fetchPC()); break;}
        case 0xcc: {call(f.getZ()==1); break;}
        case 0xcd: {call(true); break;}
        case 0xce: {adci(); break;}
        case 0xcf: {rst(1); break;}

        case 0xd0: {ret(f.getC()==0); break;}
        case 0xd1: {pop(de); break;}
        case 0xd2: {jp(f.getC()==0); break;}
        case 0xd3: {break;} //invalid
        case 0xd4: {call(f.getC()==0); break;}
        case 0xd5: {push(de); break;}
        case 0xd6: {subi(); break;}
        case 0xd7: {rst(2); break;}
        
        case 0xd8: {ret(f.getC()==1); break;}
        case 0xd9: {ret(); break;} //TODO add reti
        case 0xda: {jp(f.getC()==1); break;}
        case 0xdb: {break;} //invalid
        case 0xdc: {call(f.getC()==1); break;}
        case 0xdd: {break;} //invalid
        case 0xde: {sbci(); break;}
        case 0xdf: {rst(3); break;}

        case 0xe0: {sw_A_imm(fetchPC()); break;}
        case 0xe1: {pop(hl); break;}
        case 0xe2: {sw_A_imm(c.get()); break;}
        case 0xe3: {break;} //invalid
        case 0xe4: {break;} //invalid
        case 0xe5: {push(hl); break;}
        case 0xe6: {andi(); break;}
        case 0xe7: {rst(4); break;}
        
        case 0xe8: {add_SP_imm(); break;}
        case 0xe9: {jp_HL(); break;}
        case 0xea: {sw_A_imm16(); break;}
        case 0xeb: {break;} //invalid
        case 0xec: {break;} //invalid
        case 0xed: {break;} //invalid
        case 0xee: {xori(); break;}
        case 0xef: {rst(5); break;}

        case 0xf0: {lw_A_imm(fetchPC()); break;}
        case 0xf1: {pop(af); break;}
        case 0xf2: {lw_A_imm(c.get()); break;}
        case 0xf3: {break;} //TODO DI
        case 0xf4: {break;} //invalid
        case 0xf5: {push(af); break;}
        case 0xf6: {ori(); break;}
        case 0xf7: {rst(6); break;}

        case 0xf8: {lw_hl_sp(); break;}
        case 0xf9: {sp.set(hl.get()); cycles = 1; break;}
        case 0xfa: {lw_A_imm16(); break;}
        case 0xfb: {break;} //TODO implement EI
        case 0xfc: {break;} //invalid
        case 0xfd: {break;} //invalid
        case 0xfe: {cpi(); break;}
        case 0xff: {rst(7); break;}
    }
}

void CPU::decode_cb_inst(uint8_t instruction) {
}

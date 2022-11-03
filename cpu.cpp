#include "cpu.h"
#include "mmu.h"
#include <iostream>
#include "register.h"
#include "handler.h"

CPU::CPU(Handler& hand) : handler(hand), af(a,f), bc(b,c), de(d,e), hl(h,l) {
}

void CPU::debug2() {
    fprintf(stderr,"A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n", a.get(), f.get(), b.get(), c.get(), d.get(), e.get(), h.get(), l.get(), sp.get(), pc.get(), fetchWord(pc.get()), fetchWord(pc.get()+1), fetchWord(pc.get()+2) , fetchWord(pc.get()+3)); 
}

void CPU::debug() {
    fprintf(stderr,"af:%04x bc:%04x de:%04x hl:%04x sp:%04x pc:%04x f:%d%d%d%d\n", af.get(), bc.get(), de.get(), hl.get(), sp.get(), pc.get(), f.getZ(), f.getN(), f.getH(), f.getC()); 
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
    if (handleIF())
        return;
    if (halted) {
        wake_halt();
        return;
    }
    debug2();
    uint8_t temp = fetchPC();
    decode_inst(temp);
    //printf("%d \n", temp);
    if (IMEQueue[0]-- == 1)
        handle_IME_write();
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

//DAA
void CPU::DAA() {
    uint8_t temp = a.get();
    if (!f.getN()) {  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
        if (f.getC() || temp > 0x99) {
            temp += 0x60; 
            f.setC(true); 
        }
        if (f.getH() || (temp & 0x0f) > 0x09) { 
            temp += 0x6; 
        }
    } else {  // after a subtraction, only adjust if (half-)carry occurred
        if (f.getC()) temp-=0x60; 
        if (f.getH()) temp-=0x6; 
    }
    a.set(temp);
    f.setZ(temp == 0); // the usual z flag
    f.setH(false); // h flag is always cleared
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
void CPU::rlc(Register_8b& r) { //barrel shift register a 1 left
    uint8_t temp = r.get();
    auto carry = temp & 0x80;
    temp = temp << 1;
    r.set( (temp&0xfe) | !!carry);
    f.setC(carry);
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}
void CPU::rl(Register_8b& r) {
    auto temp = r.get();
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x80);
    temp = temp << 1;
    r.set((temp&0xfe) | flagbit);
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}
void CPU::rrc(Register_8b& r) { //barrel shift register a 1 right
    uint8_t temp = r.get();
    auto carry = temp & 0x01;
    temp = temp >> 1;
    r.set(carry << 7  | (temp&0x7f));
    f.setC(carry);
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}
void CPU::rr(Register_8b& r) {
    auto temp = r.get();
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x01);
    temp = temp >> 1;
    r.set(flagbit << 7  | (temp&0x7f));
    f.setZ(r.get()==0);
    f.setN(false);
    f.setH(false);
}

void CPU::sla(Register_8b& r) {
    auto temp = r.get();
    f.setC(temp & 0x80);
    r.set(temp << 1);
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}

void CPU::sra(Register_8b& r) {
    auto temp = r.get();
    f.setC(temp & 0x01);
    r.set((temp >> 1) | (temp&0x80));
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}

void CPU::srl(Register_8b& r) {
    auto temp = r.get();
    f.setC(temp & 0x01);
    r.set(temp >> 1);
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
}

void CPU::swap(Register_8b& r) {
    auto temp = r.get();
    r.set((temp>>4) | (temp<<4));
    f.setZ(r.get() == 0);
    f.setN(false);
    f.setH(false);
    f.setC(false);
}

//shift HL
void CPU::rlc_HL() { //barrel shift register a 1 left
    uint8_t temp = fetchWord(hl.get());
    auto carry = temp & 0x80;
    temp = temp << 1;
    temp = (temp&0xfe) | !!carry;
    storeWord(hl.get(), temp);
    f.setC(carry);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}
void CPU::rl_HL() {
    auto temp = fetchWord(hl.get());
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x80);
    temp = temp << 1;
    temp = ((temp&0xfe) | flagbit);
    storeWord(hl.get(), temp);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}
void CPU::rrc_HL() { //barrel shift register a 1 right
    uint8_t temp = fetchWord(hl.get());
    auto carry = temp & 0x01;
    temp = temp >> 1;
    temp = (carry << 7  | (temp&0x7f));
    storeWord(hl.get(), temp);
    f.setC(carry);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}
void CPU::rr_HL() {
    uint8_t temp = fetchWord(hl.get());
    uint8_t flagbit = f.getC();
    f.setC(temp & 0x01);
    temp = temp >> 1;
    temp = (flagbit << 7  | (temp&0x7f));
    storeWord(hl.get(), temp);
    f.setZ(temp==0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}

void CPU::sla_HL() {
    uint8_t temp = fetchWord(hl.get());
    f.setC(temp & 0x80);
    temp = (temp << 1);
    storeWord(hl.get(), temp);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}

void CPU::sra_HL() {
    auto temp = fetchWord(hl.get());
    f.setC(temp & 0x01);
    temp = (temp >> 1) | (temp&0x80);
    storeWord(hl.get(), temp);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}   

void CPU::srl_HL() {
    auto temp = fetchWord(hl.get());
    f.setC(temp & 0x01);
    temp = (temp >> 1);
    storeWord(hl.get(), temp);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    cycles = 2;
}

void CPU::swap_HL() {
    auto temp = fetchWord(hl.get());
    temp = ((temp>>4) | (temp<<4));
    storeWord(hl.get(), temp);
    f.setZ(temp == 0);
    f.setN(false);
    f.setH(false);
    f.setC(false);
    cycles = 2;
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
    int result = temp2 + imm;
    int carrybits = (temp2 ^ imm ^ (result & 0xFFFF));
 
    f.setH(carrybits & 0x10);
    //f.setH((((temp2&0xff)+(imm&0xff)) & 0x0100));
    f.setZ(false);
    f.setN(false);
    f.setC(carrybits & 0x100);
    //uint32_t carry = (temp2 + imm) & 0x00010000;
    //f.setC(carry);
    hl.set(result & 0xffff);
    cycles = 2;
}

   
//add
void CPU::add_HL(pairRegister& r) {
    uint16_t HL_val = hl.get();
    uint16_t r_val = r.get();
    f.setH((((HL_val&0xfff)+(r_val&0xfff)) & 0x1000));
    f.setN(false);
    uint32_t carry = (HL_val + r_val) & 0x10000;
    f.setC(carry);
    hl.set(HL_val+r_val);
    cycles = 1;
}

void CPU::add_HLSP() {
    uint16_t HL_val = hl.get();
    uint16_t SP_val = sp.get();
    f.setH((((HL_val&0xfff)+(SP_val&0xfff)) & 0x1000));
    f.setN(false);
    uint32_t carry = (HL_val + SP_val) & 0x10000;
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
    int result = temp2 + imm;
    int carrybits = (temp2 ^ imm ^ (result & 0xFFFF));
    
    f.setH(carrybits & 0x10);
    //f.setH((((temp2&0xfff)+(imm&0xfff)) & 0x01000));
    f.setZ(false);
    f.setN(false);
    f.setC(carrybits & 0x100);
    //uint32_t carry = (temp2 + imm) & 0x00010000;
    //f.setC(carry);
    sp.set(result);
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
    int r1_val = r1.get();
    f.setH(((r1_val&0x0f)-(r2_val&0x0f)) < 0);
    bool carry = (r1_val - r2_val)<0; 
    f.setN(true);
    f.setZ(r1_val - r2_val == 0);
    f.setC(carry);
    r1.set((r1_val - r2_val)&0xff);
}

void CPU::sbc(Register_8b& r1, uint8_t r2_val) {
    int r1_val = r1.get();
    int carryIn = f.getC();
    f.setH(((r1_val&0x0f)-(r2_val&0x0f)-carryIn) < 0);
    bool carry = ((r1_val - r2_val - carryIn) < 0); 
    f.setN(true);
    f.setZ(((r1_val - r2_val - carryIn)&0xff) == 0);
    f.setC(carry);
    r1.set((r1_val - r2_val - carryIn)&0xff);
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
    else {
        pc.set(pc.get()+2);    
        cycles = 2;
    }
}

void CPU::call(bool cond) {
    cycles = 2;
    uint16_t temp = fetchPC() | fetchPC() << 8;
    if (cond) {
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
    pc.set(temp);
    cycles = 2;
}

void CPU::reti() {
    ret();
    handler.mmu.IME = true;
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
    f.setZ(!(r1_val & r2_val));
    f.setN(false);
    f.setH(true);
    f.setC(false);
    r1.set(r1_val & r2_val);
}

void CPU::bitwise_xor(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ(!(r1_val ^ r2_val));
    f.setN(false);
    f.setH(false);
    f.setC(false);
    r1.set(r1_val ^ r2_val);
}

void CPU::bitwise_or(Register_8b& r1, uint8_t r2_val) {
    uint8_t r1_val = r1.get();
    f.setZ(!(r1_val | r2_val));
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
}

//cb bitwise
void CPU::bit(uint8_t rval, int bitIndex) {
    f.setZ(~(rval>>bitIndex)&0x01);
    f.setN(false);
    f.setH(true);
}

void CPU::res(Register_8b& r, int bitIndex) {
    r.set(r.get() & ~(0x01 << bitIndex));
}

void CPU::res_HL(int bitIndex) {
    uint8_t temp = fetchWord(hl.get());
    temp &= ~(0x01 << bitIndex);
    storeWord(hl.get(), temp);
    cycles = 2;
}

void CPU::set(Register_8b& r, int bitIndex) {
    r.set(r.get() | (0x01 << bitIndex));
}

void CPU::set_HL(int bitIndex) {
    uint8_t temp = fetchWord(hl.get());
    temp |= (0x01 << bitIndex);
    storeWord(hl.get(), temp);
    cycles = 2;
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

//interrupts
void CPU::halt() {
    halted = true;
}

void CPU::wake_halt() {
    uint8_t ie = handler.mmu.loadWord(0xffff);
    uint8_t iflag = handler.mmu.loadWord(0xff0f);
    uint8_t temp = ie&iflag;
    if (temp)
        halted = false;
}
void CPU::EI() {
    IMEQueue[0] = 2;//ime has 1 instruction delay
    IMEQueue[1] = true;
}

void CPU::DI() {
    IMEQueue[0] = 2;
    IMEQueue[1] = false;
}

void CPU::handle_IME_write() {
    handler.mmu.IME = IMEQueue[1];
    IMEQueue[0] = 0;
}

bool CPU::handleIF() {
    uint8_t ie = handler.mmu.loadWord(0xffff);
    uint8_t iflag = handler.mmu.loadWord(0xff0f);
    uint8_t temp = ie&iflag;
    if (handler.mmu.IME) {   
        for (int i=0; i<5; i++) {
            if ((temp>>i)&0x1) {
                handler.mmu.storeWord(0xff0f, iflag & ~(1<<i));
                handler.mmu.IME = false;
                push_PC();
                pc.set(0x0040 + i*0x8);
                cycles = 4;
                return true;
            }
        }
    }
    return false;
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
        case 0x07: {rlc(a); f.setZ(false); break;}
        
        case 0x08: {sw_sp(); break;}
        case 0x09: {add_HL(bc); break;}
        case 0x0a: {loadWord(bc.get(), a); break;}
        case 0x0b: {dec(bc); break;}
        case 0x0c: {inc(c); break;}
        case 0x0d: {dec(c); break;}
        case 0x0e: {loadimm(c); break;}
        case 0x0f: {rrc(a); f.setZ(false);  break;}

        case 0x10: {break;} //TODO: stop
        case 0x11: {loadimm(de); break;}
        case 0x12: {storeWord(de.get(), a.get()); break;}
        case 0x13: {inc(de); break;}
        case 0x14: {inc(d); break;}
        case 0x15: {dec(d); break;}
        case 0x16: {loadimm(d); break;}
        case 0x17: {rl(a); f.setZ(false); break;}
        
        case 0x18: {jr(); break;}
        case 0x19: {add_HL(de); break;}
        case 0x1a: {loadWord(de.get(), a); break;}
        case 0x1b: {dec(de); break;}
        case 0x1c: {inc(e); break;}
        case 0x1d: {dec(e); break;}
        case 0x1e: {loadimm(e); break;}
        case 0x1f: {rr(a); f.setZ(false); break;}

        case 0x20: {jr(f.getZ()==0); break;}
        case 0x21: {loadimm(hl); break;}
        case 0x22: {storeWord(hl.get(), a.get()); hl.set(hl.get()+1); break;}
        case 0x23: {inc(hl); break;}
        case 0x24: {inc(h); break;}
        case 0x25: {dec(h); break;}
        case 0x26: {loadimm(h); break;}
        case 0x27: {DAA(); break;} 

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
        case 0x76: {halt(); break;} 
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
        case 0xc4: {call(f.getZ()==0); break;} 
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
        case 0xd9: {reti(); break;} 
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
        case 0xf3: {DI(); break;} 
        case 0xf4: {break;} //invalid
        case 0xf5: {push(af); break;}
        case 0xf6: {ori(); break;}
        case 0xf7: {rst(6); break;}

        case 0xf8: {lw_hl_sp(); break;}
        case 0xf9: {sp.set(hl.get()); cycles = 1; break;}
        case 0xfa: {lw_A_imm16(); break;}
        case 0xfb: {EI(); break;} 
        case 0xfc: {break;} //invalid
        case 0xfd: {break;} //invalid
        case 0xfe: {cpi(); break;}
        case 0xff: {rst(7); break;}
    }
}

void CPU::decode_cb_inst(uint8_t instruction) {
    switch (instruction) {
        case 0x00: {rlc(b); break;}
        case 0x01: {rlc(c); break;}
        case 0x02: {rlc(d); break;}
        case 0x03: {rlc(e); break;}
        case 0x04: {rlc(h); break;}
        case 0x05: {rlc(l); break;}
        case 0x06: {rlc_HL(); break;}
        case 0x07: {rlc(a); break;}

        case 0x08: {rrc(b); break;}
        case 0x09: {rrc(c); break;}
        case 0x0a: {rrc(d); break;}
        case 0x0b: {rrc(e); break;}
        case 0x0c: {rrc(h); break;}
        case 0x0d: {rrc(l); break;}
        case 0x0e: {rrc_HL(); break;}
        case 0x0f: {rrc(a); break;}

        case 0x10: {rl(b); break;}
        case 0x11: {rl(c); break;}
        case 0x12: {rl(d); break;}
        case 0x13: {rl(e); break;}
        case 0x14: {rl(h); break;}
        case 0x15: {rl(l); break;}
        case 0x16: {rl_HL(); break;}
        case 0x17: {rl(a); break;}

        case 0x18: {rr(b); break;}
        case 0x19: {rr(c); break;}
        case 0x1a: {rr(d); break;}
        case 0x1b: {rr(e); break;}
        case 0x1c: {rr(h); break;}
        case 0x1d: {rr(l); break;}
        case 0x1e: {rr_HL(); break;}
        case 0x1f: {rr(a); break;}

        case 0x20: {sla(b); break;}
        case 0x21: {sla(c); break;}
        case 0x22: {sla(d); break;}
        case 0x23: {sla(e); break;}
        case 0x24: {sla(h); break;}
        case 0x25: {sla(l); break;}
        case 0x26: {sla_HL(); break;}
        case 0x27: {sla(a); break;}

        case 0x28: {sra(b); break;}
        case 0x29: {sra(c); break;}
        case 0x2a: {sra(d); break;}
        case 0x2b: {sra(e); break;}
        case 0x2c: {sra(h); break;}
        case 0x2d: {sra(l); break;}
        case 0x2e: {sra_HL(); break;}
        case 0x2f: {sra(a); break;}
  
        case 0x30: {swap(b); break;}
        case 0x31: {swap(c); break;}
        case 0x32: {swap(d); break;}
        case 0x33: {swap(e); break;}
        case 0x34: {swap(h); break;}
        case 0x35: {swap(l); break;}
        case 0x36: {swap_HL(); break;}
        case 0x37: {swap(a); break;}

        case 0x38: {srl(b); break;}
        case 0x39: {srl(c); break;}
        case 0x3a: {srl(d); break;}
        case 0x3b: {srl(e); break;}
        case 0x3c: {srl(h); break;}
        case 0x3d: {srl(l); break;}
        case 0x3e: {srl_HL(); break;}
        case 0x3f: {srl(a); break;}
 
        case 0x40: {bit(b.get(),0); break;}
        case 0x41: {bit(c.get(),0); break;}
        case 0x42: {bit(d.get(),0); break;}
        case 0x43: {bit(e.get(),0); break;}
        case 0x44: {bit(h.get(),0); break;}
        case 0x45: {bit(l.get(),0); break;}
        case 0x46: {bit(fetchWord(hl.get()),0); break;}
        case 0x47: {bit(a.get(),0); break;}

        case 0x48: {bit(b.get(),1); break;}
        case 0x49: {bit(c.get(),1); break;}
        case 0x4a: {bit(d.get(),1); break;}
        case 0x4b: {bit(e.get(),1); break;}
        case 0x4c: {bit(h.get(),1); break;}
        case 0x4d: {bit(l.get(),1); break;}
        case 0x4e: {bit(fetchWord(hl.get()),1); break;}
        case 0x4f: {bit(a.get(),1); break;}
  
        case 0x50: {bit(b.get(),2); break;}
        case 0x51: {bit(c.get(),2); break;}
        case 0x52: {bit(d.get(),2); break;}
        case 0x53: {bit(e.get(),2); break;}
        case 0x54: {bit(h.get(),2); break;}
        case 0x55: {bit(l.get(),2); break;}
        case 0x56: {bit(fetchWord(hl.get()),2); break;}
        case 0x57: {bit(a.get(),2); break;}

        case 0x58: {bit(b.get(),3); break;}
        case 0x59: {bit(c.get(),3); break;}
        case 0x5a: {bit(d.get(),3); break;}
        case 0x5b: {bit(e.get(),3); break;}
        case 0x5c: {bit(h.get(),3); break;}
        case 0x5d: {bit(l.get(),3); break;}
        case 0x5e: {bit(fetchWord(hl.get()),3); break;}
        case 0x5f: {bit(a.get(),3); break;}

        case 0x60: {bit(b.get(),4); break;}
        case 0x61: {bit(c.get(),4); break;}
        case 0x62: {bit(d.get(),4); break;}
        case 0x63: {bit(e.get(),4); break;}
        case 0x64: {bit(h.get(),4); break;}
        case 0x65: {bit(l.get(),4); break;}
        case 0x66: {bit(fetchWord(hl.get()),4); break;}
        case 0x67: {bit(a.get(),4); break;}

        case 0x68: {bit(b.get(),5); break;}
        case 0x69: {bit(c.get(),5); break;}
        case 0x6a: {bit(d.get(),5); break;}
        case 0x6b: {bit(e.get(),5); break;}
        case 0x6c: {bit(h.get(),5); break;}
        case 0x6d: {bit(l.get(),5); break;}
        case 0x6e: {bit(fetchWord(hl.get()),5); break;}
        case 0x6f: {bit(a.get(),5); break;}

        case 0x70: {bit(b.get(),6); break;}
        case 0x71: {bit(c.get(),6); break;}
        case 0x72: {bit(d.get(),6); break;}
        case 0x73: {bit(e.get(),6); break;}
        case 0x74: {bit(h.get(),6); break;}
        case 0x75: {bit(l.get(),6); break;}
        case 0x76: {bit(fetchWord(hl.get()),6); break;}
        case 0x77: {bit(a.get(),6); break;}

        case 0x78: {bit(b.get(),7); break;}
        case 0x79: {bit(c.get(),7); break;}
        case 0x7a: {bit(d.get(),7); break;}
        case 0x7b: {bit(e.get(),7); break;}
        case 0x7c: {bit(h.get(),7); break;}
        case 0x7d: {bit(l.get(),7); break;}
        case 0x7e: {bit(fetchWord(hl.get()),7); break;}
        case 0x7f: {bit(a.get(),7); break;}

        case 0x80: {res(b,0); break;}
        case 0x81: {res(c,0); break;}
        case 0x82: {res(d,0); break;}
        case 0x83: {res(e,0); break;}
        case 0x84: {res(h,0); break;}
        case 0x85: {res(l,0); break;}
        case 0x86: {res_HL(0); break;}
        case 0x87: {res(a,0); break;}

        case 0x88: {res(b,1); break;}
        case 0x89: {res(c,1); break;}
        case 0x8a: {res(d,1); break;}
        case 0x8b: {res(e,1); break;}
        case 0x8c: {res(h,1); break;}
        case 0x8d: {res(l,1); break;}
        case 0x8e: {res_HL(1); break;}
        case 0x8f: {res(a,1); break;}
  
        case 0x90: {res(b,2); break;}
        case 0x91: {res(c,2); break;}
        case 0x92: {res(d,2); break;}
        case 0x93: {res(e,2); break;}
        case 0x94: {res(h,2); break;}
        case 0x95: {res(l,2); break;}
        case 0x96: {res_HL(2); break;}
        case 0x97: {res(a,2); break;}

        case 0x98: {res(b,3); break;}
        case 0x99: {res(c,3); break;}
        case 0x9a: {res(d,3); break;}
        case 0x9b: {res(e,3); break;}
        case 0x9c: {res(h,3); break;}
        case 0x9d: {res(l,3); break;}
        case 0x9e: {res_HL(3); break;}
        case 0x9f: {res(a,3); break;}

        case 0xa0: {res(b,4); break;}
        case 0xa1: {res(c,4); break;}
        case 0xa2: {res(d,4); break;}
        case 0xa3: {res(e,4); break;}
        case 0xa4: {res(h,4); break;}
        case 0xa5: {res(l,4); break;}
        case 0xa6: {res_HL(4); break;}
        case 0xa7: {res(a,4); break;}

        case 0xa8: {res(b,5); break;}
        case 0xa9: {res(c,5); break;}
        case 0xaa: {res(d,5); break;}
        case 0xab: {res(e,5); break;}
        case 0xac: {res(h,5); break;}
        case 0xad: {res(l,5); break;}
        case 0xae: {res_HL(5); break;}
        case 0xaf: {res(a,5); break;}

        case 0xb0: {res(b,6); break;}
        case 0xb1: {res(c,6); break;}
        case 0xb2: {res(d,6); break;}
        case 0xb3: {res(e,6); break;}
        case 0xb4: {res(h,6); break;}
        case 0xb5: {res(l,6); break;}
        case 0xb6: {res_HL(6); break;}
        case 0xb7: {res(a,6); break;}

        case 0xb8: {res(b,7); break;}
        case 0xb9: {res(c,7); break;}
        case 0xba: {res(d,7); break;}
        case 0xbb: {res(e,7); break;}
        case 0xbc: {res(h,7); break;}
        case 0xbd: {res(l,7); break;}
        case 0xbe: {res_HL(7); break;}
        case 0xbf: {res(a,7); break;}
 
        case 0xc0: {set(b,0); break;}
        case 0xc1: {set(c,0); break;}
        case 0xc2: {set(d,0); break;}
        case 0xc3: {set(e,0); break;}
        case 0xc4: {set(h,0); break;}
        case 0xc5: {set(l,0); break;}
        case 0xc6: {set_HL(0); break;}
        case 0xc7: {set(a,0); break;}

        case 0xc8: {set(b,1); break;}
        case 0xc9: {set(c,1); break;}
        case 0xca: {set(d,1); break;}
        case 0xcb: {set(e,1); break;}
        case 0xcc: {set(h,1); break;}
        case 0xcd: {set(l,1); break;}
        case 0xce: {set_HL(1); break;}
        case 0xcf: {set(a,1); break;}
  
        case 0xd0: {set(b,2); break;}
        case 0xd1: {set(c,2); break;}
        case 0xd2: {set(d,2); break;}
        case 0xd3: {set(e,2); break;}
        case 0xd4: {set(h,2); break;}
        case 0xd5: {set(l,2); break;}
        case 0xd6: {set_HL(2); break;}
        case 0xd7: {set(a,2); break;}

        case 0xd8: {set(b,3); break;}
        case 0xd9: {set(c,3); break;}
        case 0xda: {set(d,3); break;}
        case 0xdb: {set(e,3); break;}
        case 0xdc: {set(h,3); break;}
        case 0xdd: {set(l,3); break;}
        case 0xde: {set_HL(3); break;}
        case 0xdf: {set(a,3); break;}

        case 0xe0: {set(b,4); break;}
        case 0xe1: {set(c,4); break;}
        case 0xe2: {set(d,4); break;}
        case 0xe3: {set(e,4); break;}
        case 0xe4: {set(h,4); break;}
        case 0xe5: {set(l,4); break;}
        case 0xe6: {set_HL(4); break;}
        case 0xe7: {set(a,4); break;}

        case 0xe8: {set(b,5); break;}
        case 0xe9: {set(c,5); break;}
        case 0xea: {set(d,5); break;}
        case 0xeb: {set(e,5); break;}
        case 0xec: {set(h,5); break;}
        case 0xed: {set(l,5); break;}
        case 0xee: {set_HL(5); break;}
        case 0xef: {set(a,5); break;}

        case 0xf0: {set(b,6); break;}
        case 0xf1: {set(c,6); break;}
        case 0xf2: {set(d,6); break;}
        case 0xf3: {set(e,6); break;}
        case 0xf4: {set(h,6); break;}
        case 0xf5: {set(l,6); break;}
        case 0xf6: {set_HL(6); break;}
        case 0xf7: {set(a,6); break;}

        case 0xf8: {set(b,7); break;}
        case 0xf9: {set(c,7); break;}
        case 0xfa: {set(d,7); break;}
        case 0xfb: {set(e,7); break;}
        case 0xfc: {set(h,7); break;}
        case 0xfd: {set(l,7); break;}
        case 0xfe: {set_HL(7); break;}
        case 0xff: {set(a,7); break;} 
    }
    cycles++;
}

#pragma once
#include "register.h"
#include <stdint.h>

class Handler;

class CPU {
public:
    CPU(Handler& handler);
    void init();
    void tick();
private:
    int cycles;
    Register_8b a, b, c, d, e, h, l;
    flagRegister f;
    pairRegister af, bc, de, hl;
    Register_16b pc, sp;
    Handler& handler;
    void decode_inst(uint8_t instruction);
    uint8_t fetchPC();
    uint8_t fetchWord(uint16_t addr);

    void NOP();
    void loadimm(Register_8b r);
    void loadimm(Register_16b r);
    void loadimm(pairRegister r);
    void storeWord(uint16_t addr, uint8_t data);
    
    void inc(pairRegister r);
    void inc(Register_8b r);
    void inc(Register_16b r);
    void inc_HL();

    void dec(pairRegister r);
    void dec(Register_8b r);
    void dec(Register_16b r);
    void dec_HL();

    void rlca();
    void rla();
    void rrca();
    void rra();
    
    void sw_sp();
    void sw_HL_imm();
    
    void loadWord(uint16_t addr, Register_8b r);

    void add_HL(pairRegister r);
    void add_HLSP();
    void add(Register_8b r1, uint8_t r2);
    void adc(Register_8b r1, uint8_t r2);
    void sub(Register_8b r1, uint8_t r2);
    void sbc(Register_8b r1, uint8_t r2);
    void bitwise_and(Register_8b r1, uint8_t r2);
    void bitwise_xor(Register_8b r1, uint8_t r2);
    void bitwise_or(Register_8b r1, uint8_t r2);
    void bitwise_cp(Register_8b r1, uint8_t r2);
     
    void jr();
    void jr(bool cond);
    void ret();
    void ret(bool cond);
    void jp();
    void jp(bool cond);

    void pop(pairRegister r);
    void pop_PC();

    void cpl();
    void ccf();
    void scf();

};

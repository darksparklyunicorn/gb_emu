#pragma once
#include "register.h"
#include <stdint.h>

class Handler;

class CPU {
public:
    CPU(Handler& handler);
    void init();
    void tick();
    void debug();
    void debug2();
private:
    int cycles;
    Handler& handler;
    flagRegister f;
    Register_8b a, b, c, d, e, h, l;
    pairRegister af, bc, de, hl;
    Register_16b pc, sp;
    void decode_inst(uint8_t instruction);
    void decode_cb_inst(uint8_t);
    uint8_t fetchPC();
    uint8_t fetchWord(uint16_t addr);

    void DAA();
    void NOP();
    void loadimm(Register_8b& r);
    void loadimm(Register_16b& r);
    void loadimm(pairRegister& r);
    void storeWord(uint16_t addr, uint8_t data);
    
    void inc(pairRegister& r);
    void inc(Register_8b& r);
    void inc(Register_16b& r);
    void inc_HL();

    void dec(pairRegister& r);
    void dec(Register_8b& r);
    void dec(Register_16b& r);
    void dec_HL();

    void rlc(Register_8b&);
    void rl(Register_8b&);
    void rrc(Register_8b&);
    void rr(Register_8b&);
    void sla(Register_8b&);
    void sra(Register_8b&);
    void srl(Register_8b&);
    void swap(Register_8b&);

    void rlc_HL();
    void rl_HL();
    void rrc_HL();
    void rr_HL();
    void sla_HL();
    void sra_HL();
    void srl_HL();
    void swap_HL();

    void bit(uint8_t, int);
    void res(Register_8b&, int);
    void res_HL(int);
    void set(Register_8b&, int);
    void set_HL(int);

    void sw_sp();
    void sw_HL_imm();
    void sw_A_imm(uint8_t);
    void sw_A_imm16();
    
    
    void loadWord(uint16_t addr, Register_8b& r);
    void lw_A_imm(uint8_t);
    void lw_A_imm16();
    void lw_hl_sp();

    void add_HL(pairRegister& r);
    void add_HLSP();
    void add_SP_imm();
    void add(Register_8b& r1, uint8_t r2);
    void adc(Register_8b& r1, uint8_t r2);
    void sub(Register_8b& r1, uint8_t r2);
    void sbc(Register_8b& r1, uint8_t r2);
    void bitwise_and(Register_8b& r1, uint8_t r2);
    void bitwise_xor(Register_8b& r1, uint8_t r2);
    void bitwise_or(Register_8b& r1, uint8_t r2);
    void bitwise_cp(Register_8b& r1, uint8_t r2);
     
    void jr();
    void jr(bool cond);
    void ret();
    void ret(bool cond);
    void jp();
    void jp_HL();
    void jp(bool cond);
    void call(bool);


    void pop(pairRegister& r);
    void pop_PC();
    void push(pairRegister&);
    void push_PC();

    void cpl();
    void ccf();
    void scf();
    void rst(uint8_t);

    void addi();
    void subi();
    void adci();
    void sbci();
    void andi();
    void ori();
    void xori();
    void cpi();
    
};

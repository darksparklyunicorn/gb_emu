#include <stdint.h>
#include "register.h"
#include "ppu.h"
#include "handler.h"
#include <string.h>
#define LCDC 0x40
#define SCY 0x42
#define SCX 0x43
#define LY  0x44
#define BGP 0x47
#define STAT 0x41

PPU::PPU(Handler& h) : handler(h), ioreg{}, pstate(1), fetcher(*this), x(0) {
}


void PPU::init() {
    /*    ioreg[0x00].set(0xcf);
          ioreg[0x01].set(0x00);
          ioreg[0x02].set(0x7e);
          ioreg[0x04].set(0xab);
          ioreg[0x05].set(0x00);
          ioreg[0x06].set(0x00);
          ioreg[0x07].set(0xf8);
          ioreg[0x0f].set(0xe1);
          ioreg[0x40].set(0x91);//lcdc
          ioreg[0x41].set(0x85);//stat
          ioreg[0x42].set(0x00);//scy
          ioreg[0x43].set(0x00);//scx
          ioreg[0x44].set(0x00);//ly
          ioreg[0x45].set(0x00);//lyc
          ioreg[0x47].set(0xfc);//bgp
          */
    ioreg[0x00].set(0xff);
    dots = 0;
    tcounts = 0;
    clockfrac = 1024;
    std::fill(videobuf, videobuf+(160*144*4), 120);
    fetcher.init();
}

void PPU::bitsetRegister(uint16_t addr, bool v, int index) {
    ioreg[addr].bitset(index, v);
}

uint8_t PPU::getRegister(uint16_t addr) {
    if (addr == 0x00)
        return 0xcf;
    return ioreg[addr].get();
}

void PPU::setRegister(uint16_t addr, uint8_t v) {
    ioreg[addr].set(v);
    if (addr == 0x04)
        ioreg[addr].set(0);
    if (addr == 0x07) {
        uint8_t temp = v&0x03;
        switch (temp) {
            case 0: {clockfrac = 1024; break;}
            case 1: {clockfrac = 16; break;}
            case 2: {clockfrac = 64; break;}
            case 3: {clockfrac = 256; break;}
        }
    }
}

void PPU::timer_tick() {
    if (++divcounts >= 256) {
        ioreg[0x04].set(ioreg[0x04].get()+1);
        divcounts = 0;
    }
    if (++tcounts >= clockfrac && ioreg[0x07].bitget(2)) {
        ioreg[0x05].set(ioreg[0x05].get()+1);
        tcounts = 0;
        if (ioreg[0x05].get() == 0) {
            ioreg[0x05].set(ioreg[0x06].get());
            ioreg[0x0f].bitset(2,1);
        }
    }
}

void PPU::statmode(int mode) {
    ioreg[STAT].bitset(1,mode&2);
    ioreg[STAT].bitset(0,mode&1);
}

void PPU::pixel_tick() {

    if (!ioreg[0x40].bitget(7)) 
        return;
    dots++;
    switch (pstate) {
        case 1: { //oam search
                    statmode(2);
                    if (dots < 80)
                        break;
                    pstate = 2;
                    fetcher.init();
                    break;
                }
        case 2: {
                    statmode(3);
                    fetcher.tick();
                    if (fetcher.pixelFIFO.size() <= 8)
                        return;
                    else {
                        uint8_t pixelData = fetcher.pixelFIFO.front();
                        fetcher.pixelFIFO.pop();
                        pixelData = 0xff - ((ioreg[BGP].get() >> (2*pixelData)) & 0x3) *85;
                        videobuf[(ioreg[LY].get()*160 + x)*4] = pixelData;
                        videobuf[(ioreg[LY].get()*160 + x)*4 +1] = pixelData;
                        videobuf[(ioreg[LY].get()*160 + x)*4 +2] = pixelData;
                        videobuf[(ioreg[LY].get()*160 + x)*4 +3] = 0xff;
                        /*
                           fprintf(stderr, "%d, %d, %d, %d\n", videobuf[(ioreg[LY].get()*160 + x)*4], 
                           videobuf[(ioreg[LY].get()*160 + x)*4 +1],
                           videobuf[(ioreg[LY].get()*160 + x)*4 +2],
                           videobuf[(ioreg[LY].get()*160 + x)*4 +3]);
                           */


                        x++;
                        if (x == 160) {
                            pstate = 3;
                            x=0;
                        }
                    }
                    break;
                }
        case 3: {
                    statmode(0);
                    if (dots < 456)
                        break;
                    ioreg[LY].set(ioreg[LY].get()+1);
                    pstate = 1;
                    x = 0;
                    dots = 0;

                    if (ioreg[LY].get() >= 144) {
                        pstate = 4;
                        ioreg[0x0f].bitset(0,1);
                    }
                    break; 
                }
        case 4: {
                    statmode(1);
                    if (dots >= 456) {
                        ioreg[LY].set(ioreg[LY].get()+1);
                        dots = 0;
                        if (ioreg[LY].get() >= 154) {
                            pstate = 1;
                            ioreg[LY].set(0);
                            hasNewFrame = true;
                            ioreg[0x0f].bitset(0,0);
                        }
                    }
                    break;
                }
    }

    //update interrupts

    ioreg[STAT].bitset(2,ioreg[LY].get() == ioreg[0x45].get());
}

Fetcher::Fetcher(PPU& p) : ppu(p), state(1) {
}

void Fetcher::init() {
    tilecount = 0;
    //x = ppu.getRegister(SCX)/8 &0x1f;
    y = (ppu.getRegister(LY) + ppu.getRegister(SCY))&0xff;
    y_tileRow = y%8;
    //tileID = ((ppu.ioreg[LCDC].bitget(3)) ? 0x9c00:0x9800) + x + (y/8) * 0x20;
    ticks = false;
    clearQ();
    state = 1;
}

void Fetcher::tick() {
    ticks = !ticks;
    if (ticks) {
        switch (state) {
            case 1: {
                        x = ( /*ppu.getRegister(SCX)/8 +*/ tilecount)&0x1f;
                        tileID = ((ppu.ioreg[LCDC].bitget(3)) ? 0x9c00:0x9800) + x + ((y/8) * 0x20);
                        id = ppu.handler.mmu.loadWord(tileID);
                        state = 2;
                        break;
                    }
            case 2: {
                        tileAddr = ((ppu.ioreg[LCDC].bitget(4))? id*16 : 0x1000 +16*((id&0x7f)-(id&0x80))) 
                            +y_tileRow*2 + 0x8000;
                        uint16_t tiledata = ppu.handler.mmu.loadWord(tileAddr);
                        for (int i=0; i<8; i++) 
                            buf[i] = (tiledata>>i) & 0x01; 
                        state = 3;
                        break;
                    }
            case 3: {
                        tileAddr++;
                        uint16_t tiledata = ppu.handler.mmu.loadWord(tileAddr);
                        for (int i=0; i<8; i++) 
                            buf[i] |= ((tiledata>>i) & 0x01) * 2; 
                        state = 4;
                        break;
                    }
            case 4: {

                        if (pixelFIFO.size() <= 8) {
                            for (int i=7; i>=0; i--)
                                pixelFIFO.push(buf[i]);
                            state = 1; 
                            tilecount++;
                        }
                        break;
                    }
        }
    }

}

void Fetcher::clearQ() {
    while (!pixelFIFO.empty())
        pixelFIFO.pop();
    //fprintf(stderr, "%d\n", pixelFIFO.size());
}

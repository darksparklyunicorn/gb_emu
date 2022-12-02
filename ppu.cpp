#include <stdint.h>
#include "register.h"
#include "ppu.h"
#include "handler.h"
#include <string.h>
#define LCDC 0x40
#define SCY 0x42
#define SCX 0x43
#define LY  0x44

PPU::PPU(Handler& h) : handler(h), ioreg{}, pstate(1), fetcher(*this) {
}

int PPU::video_callback(uint8_t *callback_buffer) {
    if (hasNewFrame) {
        memcpy(callback_buffer, videobuf, sizeof(videobuf));
        hasNewFrame = false;
        return 0;
    } 
    return -1;
}


void PPU::init() {
    ioreg[0x07].set(0xf8);//tac
    ioreg[0x0f].set(0xe1);
    ioreg[0x40].set(0x91);//lcdc
    ioreg[0x44].set(0x00);//ly
    dots = 0;
    tcounts = 0;
    clockfrac = 1024;
    fetcher.init();
}

void PPU::bitsetRegister(uint16_t addr, bool v, int index) {
    ioreg[addr].bitset(index, v);
}

uint8_t PPU::getRegister(uint16_t addr) {
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

void PPU::pixel_tick() {
    
    if (!ioreg[0x40].bitget(7))
        return;
    dots++;
    switch (pstate) {
        case 1: {
                    if (dots < 80)
                        break;
                    pstate = 2;
                    fetcher.init();
                    break;
                }
        case 2: {
                    fetcher.tick();
                    if (fetcher.pixelFIFO.empty())
                        return;
                    uint16_t pixelData = fetcher.pixelFIFO.front();
                    fetcher.pixelFIFO.pop();
                    pixelData = (handler.mmu.loadWord(pixelData) >> (2*pixelData)) & 2;
                    videobuf[(ioreg[LY].get()*160 + x)*4] = pixelData;
                    videobuf[(ioreg[LY].get()*160 + x)*4 +1] = pixelData;
                    videobuf[(ioreg[LY].get()*160 + x)*4 +2] = pixelData;
                    videobuf[(ioreg[LY].get()*160 + x)*4 +3] = pixelData;
                    x++;
                    if (x == 160) {
                        pstate = 3;
                    }
                    break;
                }
        case 3: {
                    if (dots < 456)
                        break;
                    ioreg[LY].set(ioreg[LY].get()+1);
                    pstate = 1;
                    x = 0;
                    dots = 0;

                    if (ioreg[LY].get() >= 144) {
                        pstate = 4;
                        hasNewFrame = true;
                    }
                                        break; 
                }
        case 4: {
                    if (dots == 456) {
                        ioreg[LY].set(ioreg[LY].get()+1);
                        dots = 0;
                        if (ioreg[LY].get() >= 154) {
                            pstate = 1;
                            ioreg[LY].set(0);
                        }
                    }
                    break;
                }
    }
}

Fetcher::Fetcher(PPU& p) : ppu(p), state(1) {
}

void Fetcher::init() {
    auto x = (ppu.getRegister(SCX)/8 + ppu.x/8)&0x1f;
    auto y = (ppu.getRegister(LY) + ppu.getRegister(SCY))&0xff;
    y_tileRow = y%8;
    tileID = 0x9800 + x + (y/8) * 0x20;
    ticks = false;
    clearQ(pixelFIFO);
}

void Fetcher::tick() {
    ticks = !ticks;
    if (ticks) {
        switch (state) {
            case 1: {
                        id = ppu.handler.mmu.loadWord(tileID);
                        state = 2;
                        break;
                    }
            case 2: {
                        uint16_t tileAddr = 0x8000 + id*16 + y_tileRow*2;
                        uint16_t tiledata = ppu.handler.mmu.loadWord(tileAddr);
                        for (int i=0; i<8; i++) 
                            buf[i] = (tiledata>>i) & 0x01; 
                        state = 3;
                        break;
                    }
            case 3: {
                        uint16_t tileAddr = 0x8001 + id*16 + y_tileRow*2;
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
                            tileID++;
                        }
                        break;
                    }
        }
    }

}

void Fetcher::clearQ(std::queue<int>& q) {
    while (!q.empty())
        q.pop();
}

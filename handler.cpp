#include "handler.h"
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

Handler::Handler(bool usedebug) : debug(usedebug), cpu(*this), mmu(*this), ppu(*this), ticks(0) {
}

void Handler::getTileMap(uint8_t * buf) {
    fprintf(stderr, "getting pixels");
    uint8_t palette = ppu.getRegister(0x47);
    for (int k=0; k<0x20; k++) {
  //      fprintf(stderr, "k=%d\n", k);
        for (int j=0; j<8; j++) {
            for (int i=0; i<0x20; i++) {
    //            fprintf(stderr, "i=%d\n", i);

                uint8_t tileID = mmu.loadWord(0x9800+i + 32*k);
                uint8_t pixDataLow = mmu.loadWord(0x8000 + tileID*16 + j*2);
                uint8_t pixDataHigh = mmu.loadWord(0x8001 + tileID*16 + j*2);
                for (int h=0; h<8; h++) {
//                    fprintf(stderr, "i=%d\n", i);
                    uint8_t pixval = (pixDataHigh>>(7-h)&1)*2 + (pixDataLow>>(7-h)&1);
                    uint8_t pixColor = 0xff - (((palette >> (2*pixval)) & 0x3) *85);
                    buf[((k*8+j)*256 + (i*8+h))*4] = pixColor;
                    buf[((k*8+j)*256 + (i*8+h))*4 +1] = pixColor;
                    buf[((k*8+j)*256 + (i*8+h))*4 +2] = pixColor;
                    buf[((k*8+j)*256 + (i*8+h))*4 +3] = 0xff;
                }
            }
        }
    }
}


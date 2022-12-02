#include "handler.h"
#include <stdio.h>
#include <thread>
#include <chrono>

#define CLOCKSPEED 4194304;

int main(int argc, char **argv) {
    Handler handler;
    handler.mmu.loadROM(argv[1]);
    handler.cpu.init();
    handler.mmu.init();
    handler.ppu.init();
    //handler.cpu.debug2();
    char buf[10];
    int i=0;
    for (unsigned long long j=0; j<0xfffffff; j++) {
        //std::this_thread::sleep_for (std::chrono::nanoseconds(900));
        
        handler.ppu.timer_tick();
        handler.ppu.pixel_tick();
        if (++i == 4) {
            handler.cpu.tick();
            handler.mmu.tick();
        i=0;
        }
        //scanf("%s", buf);
    }
}    

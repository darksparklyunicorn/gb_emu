#include "handler.h"
#include <stdio.h>
#include <thread>
#include <chrono>

int main(int argc, char **argv) {
    Handler handler;
    handler.mmu.loadROM(argv[1]);
    handler.cpu.init();
    handler.mmu.init();
    //handler.cpu.debug2();
    char buf[10];
    int i=0;
    for (int j=0; j<0xffff; j++) {
        std::this_thread::sleep_for (std::chrono::nanoseconds(900));
        handler.cpu.tick();
        handler.mmu.tick();
        //scanf("%s", buf);
    }
}    

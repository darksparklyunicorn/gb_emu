#include "handler.h"
#include <stdio.h>
#include <thread>
#include <chrono>

#define CLOCKSPEED 4194304;

int main(int argc, char **argv) {
    Handler handler(false);
    handler.init(argv[1]);
    uint8_t imgbuf[160*144*4];
    //handler.cpu.debug2();
    for (unsigned long long j=0; j<0x8ffffff; j++) {
        //std::this_thread::sleep_for (std::chrono::nanoseconds(900));
        handler.tick();
        if (!handler.frame_callback(imgbuf))
            fprintf(stdout, "%d", imgbuf[j%160*144*4]);
        //scanf("%s", buf);
    }
}    

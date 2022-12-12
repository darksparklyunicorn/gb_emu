#include "handler.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include <ctime>

#define CLOCKSPEED 4194304;

int main(int argc, char **argv) {
    Handler handler(false);
    handler.init(argv[1]);
    uint8_t imgbuf[160*144*4];
    //handler.cpu.debug2();
    auto start = std::chrono::system_clock::now();
    for (unsigned long long j=0; j<0x5ffffff; j++) {
        //std::this_thread::sleep_for (std::chrono::nanoseconds(900));
        handler.tick();
        /* if (!handler.frame_callback(imgbuf))
            fprintf(stderr, "%d", imgbuf[j%160*144*4]);
        //scanf("%s", buf);
        
        */
    }
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end-start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);

    std::cout << "finished computation at " << std::ctime(&end_time)
        << "elapsed time: " << elapsed_seconds.count() << "s"
        << std::endl;
}    

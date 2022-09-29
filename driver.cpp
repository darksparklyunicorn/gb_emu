#include "handler.h"

int main() {
    Handler handler;
    handler.mmu.loadROM("testfile.txt");
    for (int i=0; i<48; i++) {
        handler.cpu.tick();
    }
}    

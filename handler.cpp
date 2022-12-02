#include "handler.h"
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

Handler::Handler(bool usedebug) : debug(usedebug), cpu(*this), mmu(*this), ppu(*this), ticks(0) {
}

int Handler::frame_callback(uint8_t * buffer) {
    return ppu.video_callback(buffer);
}

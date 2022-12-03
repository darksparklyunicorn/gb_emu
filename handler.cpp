#include "handler.h"
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

Handler::Handler(bool usedebug) : debug(usedebug), cpu(*this), mmu(*this), ppu(*this), ticks(0) {
}


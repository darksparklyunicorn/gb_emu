#include "handler.h"
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

Handler::Handler() : cpu(*this), mmu(*this), ppu(*this) {
}

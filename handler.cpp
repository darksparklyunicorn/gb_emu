#include "handler.h"
#include "cpu.h"
#include "mmu.h"

Handler::Handler() : cpu(*this), mmu(*this) {
}

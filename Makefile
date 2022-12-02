ifeq ($(os),windows)
	CXX = x86_64-w64-mingw32-g++
	CXXFLAGS = -g -Wall -O3 -I/usr/local/cross-tools/x86_64-w64-mingw32/include
	LDFLAGS = -static -static-libgcc
	LIBS = `/usr/local/cross-tools/x86_64-w64-mingw32/bin/sdl2-config --static-libs` -mconsole
	FRONTT = gb_emu.exe
else
	CXX = g++
	CXXFLAGS = -g -Wall -O3
	LIBS = -lSDL2main -lSDL2
	FRONTT = gb_emu
endif

gb_emu: frontend.o cpu.o ppu.o handler.o mmu.o register.o
	$(CXX) $(LDFLAGS) frontend.o cpu.o ppu.o handler.o mmu.o register.o -o $(FRONTT) $(LIBS)
main: driver.o cpu.o ppu.o handler.o mmu.o register.o
	$(CXX) driver.o cpu.o ppu.o handler.o mmu.o register.o -o main
driver.o: driver.cpp handler.h

cpu.o: cpu.cpp cpu.h

ppu.o: ppu.cpp ppu.h

handler.o: handler.cpp handler.h

mmu.o: mmu.cpp mmu.h

register.o: register.cpp register.h

frontend.o: frontend.cpp
	$(CXX) $(CXXFLAGS) -c frontend.cpp

.PHONY: clean
clean:
	rm -f *.o
.PHONY: backend
backend: clean main

.PHONY: all
all: clean gb_emu

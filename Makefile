ifeq ($(os),windows)
	CXX = x86_64-w64-mingw32-g++
	CXXFLAGS = -g -Wall -I/usr/local/cross-tools/x86_64-w64-mingw32/include
	LDFLAGS = -static -static-libgcc
	LIBS = `/usr/local/cross-tools/x86_64-w64-mingw32/bin/sdl2-config --static-libs` -mconsole
	FRONTT = front.exe
else
	CXX = g++
	CXXFLAGS = -g -Wall
	LIBS = -lSDL2main -lSDL2
	FRONTT = front
endif

main: driver.o cpu.o ppu.o handler.o mmu.o register.o
	$(CXX) driver.o cpu.o ppu.o handler.o mmu.o register.o -o main
driver.o: driver.cpp handler.h

cpu.o: cpu.cpp cpu.h

ppu.o: ppu.cpp ppu.h

handler.o: handler.cpp handler.h

mmu.o: mmu.cpp mmu.h

register.o: register.cpp register.h

front: frontend.o
	$(CXX) $(LDFLAGS) frontend.o -o $(FRONTT) $(LIBS)
frontend.o: frontend.cpp
	$(CXX) $(CXXFLAGS) -c frontend.cpp

.PHONY: clean
clean:
	rm -f *.o
.PHONY: all
all: clean main

main: driver.o cpu.o handler.o mmu.o register.o
	g++ driver.o cpu.o handler.o mmu.o register.o -o main
driver.o: driver.cpp handler.h

cpu.o: cpu.cpp cpu.h

handler.o: handler.cpp handler.h

mmu.o: mmu.cpp mmu.h

register.o: register.cpp register.h

.PHONY: clean
clean:
	rm -f *.o
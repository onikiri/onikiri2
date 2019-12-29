CC_LINUX = ~/opt/gcc/riscv64-linux/7.1.1/bin/riscv64-unknown-linux-gnu-gcc
CC_ELF   = ~/opt/gcc/riscv64-elf/7.1.1/bin/riscv64-unknown-elf-gcc

# It is necessary to add a '-static' option for static link.
all: result_linux result_newlib

result_linux: Makefile main.c
	$(CC_LINUX) main.c -o result_linux -O0 -g -static 

result_newlib: Makefile main.c
	$(CC_ELF) main.c -o result_newlib -O0 -g -static 

clean : 
	rm result_linux result_newlib



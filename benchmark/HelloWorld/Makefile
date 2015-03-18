
AS      = /opt/gcc/alpha/4.5.3/bin/alpha-unknown-linux-gnu-as
CC      = /opt/gcc/alpha/4.5.3/bin/alpha-unknown-linux-gnu-gcc
CXX     = /opt/gcc/alpha/4.5.3/bin/alpha-unknown-linux-gnu-g++
OBJDUMP = /opt/gcc/alpha/4.5.3/bin/alpha-unknown-linux-gnu-objdump

# It is necessary to add a '-static' option for static link.
all : main.c
	$(CC) main.c -static

clean : 
	rm a.out


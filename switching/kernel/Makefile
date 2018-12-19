include ../../Makefile.common

INCPATH = -I ../../simpleio -I ../../mimg
LIBPATH = -L ../../simpleio -lio

all: kernel

#----------------------------------------------------------------------------
# A simple protected mode kernel that switches back and forth
# between user and kernel mode when a system call is invoked.

KOBJS   = init.o kernel.o
kernel: ${KOBJS} kernel.ld
	$(LD) -T kernel.ld -o kernel ${KOBJS} ${LIBPATH} --print-map > kernel.map
	strip kernel

init.o: init.s
	$(CC) -Wa,-alsm=init.lst -c -o init.o init.s

kernel.o: kernel.c
	$(CC) ${CCOPTS} ${INCPATH} -o kernel.o -c kernel.c

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r kernel *.o *.lst *.map

#----------------------------------------------------------------------------

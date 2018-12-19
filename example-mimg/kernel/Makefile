include ../../Makefile.common

INCPATH = -I ../../simpleio -I ../../mimg
LIBPATH = -L ../../simpleio -lio

all: kernel

#----------------------------------------------------------------------------
# kernel:  A trivial kernel that can be invoked by mimgload
KOBJS   = init.o kernel.o
kernel: ${KOBJS} kernel.ld
	$(LD) -T kernel.ld -o kernel ${KOBJS} ${LIBPATH} --print-map > kernel.map
	strip kernel

init.o: init.s
	$(CC) -Wa,-alsm=prot.lst -c -o init.o init.s

kernel.o: kernel.c
	$(CC) ${CCOPTS} ${INCPATH} -o kernel.o -c kernel.c

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r kernel *.o *.lst *.map

#----------------------------------------------------------------------------

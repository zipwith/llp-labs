include ../../Makefile.common

KERNEL_SPACE    = 0xc0000000      # Kernel space starts at 3GB
KERNEL_LOAD     = 0x00100000      # Kernel loads at 1MB

INCPATH = -I ../../winio -I ../../mimg
CCDEFS  = -DKERNEL_SPACE=${KERNEL_SPACE} \
          -DKERNEL_LOAD=${KERNEL_LOAD}

LIBPATH = -L ../../winio -lwinio
LDDEFS  = --defsym KERNEL_SPACE=${KERNEL_SPACE} \
          --defsym KERNEL_LOAD=${KERNEL_LOAD}

all: kernel

#----------------------------------------------------------------------------
# A simple protected mode kernel that uses paging.

KOBJS	= init.o kernel.o syscalls.o proc.o paging.o alloc.o caps.o
kernel: ${KOBJS} kernel.ld
	$(LD) ${LDDEFS} -T kernel.ld -o kernel ${KOBJS} ${LIBPATH} \
		--print-map > kernel.map
	strip kernel

init.o:		init.S memory.h
kernel.o:	kernel.c util.h context.h paging.h memory.h hardware.h alloc.h caps.h
syscalls.o:	syscalls.c util.h context.h paging.h memory.h hardware.h alloc.h caps.h
proc.o:		proc.c util.h context.h proc.h paging.h caps.h
paging.o:	paging.c util.h paging.h memory.h alloc.h
alloc.o:	alloc.c  util.h memory.h alloc.h
caps.o:		caps.c  util.h caps.h

.c.o:
	$(CC) ${CCOPTS} ${INCPATH} ${CCDEFS} -o $*.o -c $*.c

.S.o:
	$(CC) -Wa,-alsm=$*.lst ${INCPATH} ${CCDEFS} -o $*.o -c $*.S

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -r kernel *.o *.lst *.map

#----------------------------------------------------------------------------

include ../../Makefile.common

KERNEL_SPACE	= 0xc0000000      # Kernel space starts at 3GB
KERNEL_LOAD	= 0x00100000      # Kernel loads at 1MB

# Include Files:
INCPATH		= -I include -I ../../simpleio -I ../../mimg
CCDEFS		= -DKERNEL_SPACE=${KERNEL_SPACE} \
		  -DKERNEL_LOAD=${KERNEL_LOAD}

# Linker Specifications:
LIBPATH		= -L ../../simpleio -lio
LDDEFS		= --defsym KERNEL_SPACE=${KERNEL_SPACE} \
		  --defsym KERNEL_LOAD=${KERNEL_LOAD}

all:		pork
		objdump -DS pork > pork.lst

wc:
		wc -l Makefile pork.ld include/*.h *.S *.c 

# Include file rules: -----------------------------------------------------
memory.h:
context.h:
fpage.h:
kip.h:
pork.h:		kip.h
space.h:
threads.h:	kip.h space.h context.h

OBJS          = boot.o kip.o memory.o space.o \
		threads.o ipc.o scheduling.o pork.o

# Implementation file rules: ----------------------------------------------
pork:		${OBJS} pork.ld
		$(LD) ${LDDEFS} -T pork.ld -o pork ${OBJS} ${LIBPATH} \
			--print-map > pork.map
#		strip pork

boot.o:		boot.S       kip.h
kip.o:		kip.S        kip.h
memory.o:	memory.c     pork.h memory.h
space.o:	space.c      pork.h memory.h space.h
threads.o:	threads.c    pork.h memory.h threads.h
ipc.o:		ipc.c        pork.h memory.h threads.h
scheduling.o:	scheduling.c pork.h         threads.h
pork.o:		pork.c       pork.h space.h threads.h

.c.o:
		$(CC) ${CCOPTS} ${INCPATH} ${CCDEFS} -o $*.o -c $*.c

.S.o:
		$(CC) -Wa,-alsm=$*.lst ${INCPATH} ${CCDEFS} -o $*.o -c $*.S

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
		-rm -rf pork *.o *.lst *.map *.cdepn *.graph *.dot

#----------------------------------------------------------------------------

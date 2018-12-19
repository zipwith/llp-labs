#----------------------------------------------------------------------------
include Makefile.common

.phony: all run libs clean

# This Makefile can be used to automatically compile and build one of the
# demonstration kernels/labs.  To configure this, simply uncomment (i.e.,
# remove the leading "#" symbol) from the appropriate line below and make
# sure that all of the other options are commented out):

BOOT = hello
#BOOT = bootinfo
#BOOT = example-mimg
#BOOT = example-gdt
#BOOT = example-idt
#BOOT = switching
#BOOT = paging
#BOOT = pork
#BOOT = caps

all:	libs
	make -C ${BOOT}

run:	libs
	make -C ${BOOT} run

libs:
	make -C simpleio
	make -C mimg
	make -C userio
	make -C winio

clean:
	make -C hello              clean
	make -C simpleio           clean
	make -C bootinfo           clean
	make -C mimg               clean
	make -C example-mimg       clean
	make -C example-gdt        clean
	make -C example-idt        clean
	make -C switching          clean
	make -C paging             clean
	make -C pork               clean
	make -C caps               clean

#----------------------------------------------------------------------------

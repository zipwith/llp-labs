#----------------------------------------------------------------------------
# Makefile for a simple bare metal program that displays boot information
#----------------------------------------------------------------------------

include ../Makefile.common

#----------------------------------------------------------------------------
# Build rules:

all:	cdrom.iso

run:	cdrom.iso
	$(QEMU) -m 32 -serial stdio -cdrom cdrom.iso

cdrom.iso: cdrom
	grub-mkrescue -o cdrom.iso cdrom

# make a basic cdrom image
cdrom: grub.cfg bootinfo
	mkdir -p cdrom/boot/grub
	cp grub.cfg cdrom/boot/grub
	cp bootinfo cdrom
	cp boot.s bootinfo.ld bootinfo.c Makefile grub.cfg cdrom
	touch cdrom

#----------------------------------------------------------------------------
OBJS   = boot.o bootinfo.o

bootinfo: ${OBJS} bootinfo.ld
	$(LD) -T bootinfo.ld -o bootinfo ${OBJS} \
		-L ../simpleio -lio --print-map > bootinfo.map
	strip bootinfo

boot.o: boot.s
	$(CC) -Wa,-alsm=boot.lst -c -o boot.o boot.s

bootinfo.o: bootinfo.c
	$(CC) ${CCOPTS} -I ../simpleio -o bootinfo.o -c bootinfo.c

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -rf cdrom cdrom.iso bootinfo *.o *.lst *.map

#----------------------------------------------------------------------------

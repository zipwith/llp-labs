#----------------------------------------------------------------------------
# Makefile for a simple bare metal program
#----------------------------------------------------------------------------
# Basic settings:

CC	  = gcc -m32
CCOPTS    = -std=gnu99 -O -Wall -nostdlib -nostdinc -Winline \
	    -nostartfiles -nodefaultlibs -fno-builtin -fomit-frame-pointer \
	    -fno-stack-protector -freg-struct-return
LD        = ld -melf_i386
QEMU      = qemu-system-i386

#----------------------------------------------------------------------------
# Build rules:

.SUFFIXES:
.SUFFIXES: .S .s .c .cc .h .o .a .iso .img .gz .cdepn .graph

.PHONY: all clean

all:	cdrom.iso

run:	cdrom.iso
	$(QEMU) -m 32 -serial stdio -cdrom cdrom.iso

cdrom.iso: cdrom
	grub-mkrescue -o cdrom.iso cdrom

# make a basic cdrom image
cdrom: grub.cfg hello
	mkdir -p cdrom/boot/grub
	cp grub.cfg cdrom/boot/grub
	cp hello cdrom
	touch cdrom

OBJS   = boot.o hello.o

hello: ${OBJS} hello.ld
	$(LD) -T hello.ld -o hello ${OBJS} --print-map > hello.map
	strip hello

boot.o: boot.s
	$(CC) -Wa,-alsm=boot.lst -c -o boot.o boot.s

hello.o: hello.c
	$(CC) ${CCOPTS} -o hello.o -c hello.c

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	-rm -rf cdrom cdrom.iso hello *.o *.lst *.map

#----------------------------------------------------------------------------

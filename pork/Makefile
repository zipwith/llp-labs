#----------------------------------------------------------------------------
include ../Makefile.common

KERNEL = kernel

all:	cdrom.iso

run:	cdrom.iso
	$(QEMU) -m 32 -serial stdio -cdrom cdrom.iso

include ../Makefile.cdrom

image:
	make -C $(KERNEL)
	make -C user
	../mimg/mimgmake image          \
		noload:../mimg/mimgload \
		bootdata:0x0000-0x3fff  \
		$(KERNEL)/pork          \
		user/sigma0/sigma0      \
 		user/root/root
#		user/l4ka-pingpong/pingpong

#----------------------------------------------------------------------------
# tidy up after ourselves ...
clean:
	make -C $(KERNEL) clean
	make -C user      clean
	-rm -rf cdrom cdrom.iso image image.gz

#----------------------------------------------------------------------------

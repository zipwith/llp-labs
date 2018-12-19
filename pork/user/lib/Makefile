include ../../../Makefile.common

all:	libl4.a

L4OBJS = syscall.o
libl4.a: ${L4OBJS}
	ar r libl4.a ${L4OBJS}
	ranlib libl4.a

syscall.o: syscall.s
	$(CC) -Wa,-alsm=syscall.lst -o syscall.o -c syscall.s

test.s:	   test.c
	$(CC) ${CCOPTS} -c -S test.c

clean:
	-rm -r *.a *.o *.lst *.map

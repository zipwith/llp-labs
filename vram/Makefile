.SUFFIXES:	.c .h .s .o
.PHONY:		clean

OBJFILES = videoasm.o videodisp.o

video1:	video1.c video.h ${OBJFILES}
	gcc -m32 -o video1 video1.c videoasm.o videodisp.o

video2:	video2.c video.h ${OBJFILES}
	gcc -m32 -o video2 video2.c videoasm.o videodisp.o

video3:	video3.c video.h ${OBJFILES}
	gcc -m32 -o video3 video3.c videoasm.o videodisp.o

videoasm.o:	videoasm.s
	gcc -m32 -c -Wa,-alsnm=videoasm.lst videoasm.s

videodisp.o:	videodisp.c
	gcc -m32 -c videodisp.c

clean:
	-rm *.o videoasm.lst video1 video2 video3

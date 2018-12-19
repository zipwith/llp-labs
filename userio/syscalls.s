	.text

#-------------------------------------------------------------------------
# Output a single character.

        .globl  putchar
putchar:pushl   %eax
        mov     8(%esp), %eax
        int     $129
        popl    %eax
        ret

#-------------------------------------------------------------------------
# Clear the output window.

        .globl  cls
cls:	int     $130
        ret

#-------------------------------------------------------------------------
# Set the video color attribute.

        .globl  setAttr
setAttr:pushl   %eax
        mov     8(%esp), %eax
        int     $131
        popl    %eax
        ret

#-------------------------------------------------------------------------

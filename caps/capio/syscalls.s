	.text

#-------------------------------------------------------------------------
# Output a single character.

        .globl  capputchar
capputchar:
	pushl	%ecx
	mov	8(%esp), %ecx
        mov     12(%esp), %eax
        int     $129
	popl	%ecx
        ret

#-------------------------------------------------------------------------
# Clear the output window.

        .globl  capcls
capcls:	pushl	%ecx
	mov	8(%esp), %ecx
	int     $130
	popl	%ecx
        ret

#-------------------------------------------------------------------------
# Set the video color attribute.

        .globl  capsetAttr
capsetAttr:
	pushl	%ecx
	mov	8(%esp), %ecx
        mov     12(%esp), %eax
        int     $131
	popl	%ecx
        ret

#-------------------------------------------------------------------------

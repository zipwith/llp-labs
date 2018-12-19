	.file	"sigma0.c"
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align 4
.LC0:
	.string	"This is a simple sigma0 server\n"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"sigma0 utcb is at: %x\n"
	.section	.rodata.str1.4
	.align 4
.LC2:
	.string	"Now waiting for first message ...\n"
	.section	.rodata.str1.1
.LC3:
	.string	"First message from %x\n"
	.section	.rodata.str1.4
	.align 4
.LC4:
	.string	"sigma0: received msg (tag=%x) from %x\n"
	.section	.rodata.str1.1
.LC5:
	.string	"pagefault %x, mr1=%x, mr2=%x\n"
	.section	.rodata.str1.4
	.align 4
.LC6:
	.string	"Ignoring message/failure, trying again ...\n"
	.align 4
.LC7:
	.string	"succ=%d, u=%d, t=%d, tag = %x\n"
	.text
.globl cmain
	.type	cmain, @function
cmain:
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	$39, 12(%esp)
	movl	$0, 8(%esp)
	movl	$14, 4(%esp)
	movl	$10, (%esp)
	call	setWindow
	movl	$3, (%esp)
	call	setAttr
	call	cls
	movl	$.LC0, (%esp)
	call	printf
#APP
	 movl %gs:0, %eax

#NO_APP
	movl	%eax, 4(%esp)
	movl	$.LC1, (%esp)
	call	printf
	movl	$.LC2, (%esp)
	call	printf
#APP
	  movl %gs:0, %eax

	  movl %gs:0, %edx

#NO_APP
	movl	(%eax), %eax
	orb	$64, %ah
	movl	%eax, (%edx)
	leal	24(%esp), %edx
	leal	40(%esp), %eax
	movl	%eax, 12(%esp)
	movl	$-1, 8(%esp)
	movl	$0, 4(%esp)
	movl	%edx, (%esp)
	call	L4_Ipc
	subl	$4, %esp
	movl	24(%esp), %ebx
	movl	40(%esp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC3, (%esp)
	call	printf
	leal	24(%esp), %ebp
	leal	40(%esp), %edi
.L12:
	movl	40(%esp), %eax
	movl	%eax, 8(%esp)
	movl	%ebx, 4(%esp)
	movl	$.LC4, (%esp)
	call	printf
	movl	%ebx, %eax
	shrl	$15, %eax
	xorl	$1, %eax
	movl	%eax, %esi
	andl	$1, %esi
	je	.L3
	movl	%ebx, %eax
	andl	$63, %eax
	cmpl	$2, %eax
	jne	.L3
	testl	$4032, %ebx
	jne	.L3
	movl	%ebx, %eax
	shrl	$20, %eax
	cmpl	$4094, %eax
	jne	.L3
#APP
	  movl %gs:0, %eax

#NO_APP
	movl	4(%eax), %ebx
#APP
	  movl %gs:0, %eax

#NO_APP
	movl	8(%eax), %eax
	movl	%eax, 12(%esp)
	movl	%ebx, 8(%esp)
	movl	40(%esp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC5, (%esp)
	call	printf
#APP
	  movl %gs:0, %eax

#NO_APP
	movl	$128, (%eax)
#APP
	  movl %gs:0, %edx

#NO_APP
	movl	%ebx, %eax
	andl	$-4096, %eax
	orl	$8, %eax
	movl	%eax, 4(%edx)
#APP
	  movl %gs:0, %eax

#NO_APP
	andl	$-1024, %ebx
	orl	$23, %ebx
	movl	%ebx, 8(%eax)
	movl	40(%esp), %ecx
#APP
	  movl %gs:0, %eax

	  movl %gs:0, %edx

#NO_APP
	movl	(%eax), %eax
	andb	$127, %ah
	orb	$64, %ah
	movl	%eax, (%edx)
	movl	%edi, 12(%esp)
	movl	$-1, 8(%esp)
	movl	%ecx, 4(%esp)
	movl	%ebp, (%esp)
	call	L4_Ipc
	subl	$4, %esp
	movl	24(%esp), %ebx
	jmp	.L12
.L3:
	movl	$.LC6, (%esp)
	call	printf
	movl	%ebx, 16(%esp)
	movl	%ebx, %eax
	shrl	$6, %eax
	andl	$63, %eax
	movl	%eax, 12(%esp)
	movl	%ebx, %eax
	andl	$63, %eax
	movl	%eax, 8(%esp)
	movl	%esi, 4(%esp)
	movl	$.LC7, (%esp)
	call	printf
#APP
	  movl %gs:0, %eax

	  movl %gs:0, %edx

#NO_APP
	movl	(%eax), %eax
	orb	$64, %ah
	movl	%eax, (%edx)
	movl	%edi, 12(%esp)
	movl	$-1, 8(%esp)
	movl	$0, 4(%esp)
	movl	%ebp, (%esp)
	call	L4_Ipc
	subl	$4, %esp
	movl	24(%esp), %ebx
	jmp	.L12
	.size	cmain, .-cmain
	.ident	"GCC: (GNU) 4.2.4 (Ubuntu 4.2.4-1ubuntu4)"
	.section	.note.GNU-stack,"",@progbits

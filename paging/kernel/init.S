#--------------------------------------------------------------------------
# init.S:  Initialize simple kernel with paging
#
# Mark P. Jones, April 2015, 2016

#include "memory.h"

#--------------------------------------------------------------------------
# General definitions:
#--------------------------------------------------------------------------

	.set	RESERVED, 0	# Used to mark a reserved field

#--------------------------------------------------------------------------
# Initial stack:
#--------------------------------------------------------------------------

	.data
	.globl  initdir
	.align  (1<<PAGESIZE)
initdir:.space  4096            # Initial page directory

	.space	4096		# Kernel stack
stack:

#--------------------------------------------------------------------------
# Entry point:
#--------------------------------------------------------------------------

	.text
	.globl	entry
entry:	cli			# Turn off interrupts

	#------------------------------------------------------------------
	# Create initial page directory in which the initial PHYSMAP
	# portion of physical memory is mapped 1:1 and into KERNEL_SPACE.

	# Address of page dir:  (we're not in high memory yet ...)
	leal    (initdir-KERNEL_SPACE), %edi
	movl    %edi, %esi      # save in %esi

	movl    $1024, %ecx     # Zero out complete page directory
	movl    $0, %eax
1:	movl    %eax, (%edi)
	addl    $4, %edi
	decl    %ecx
	jnz     1b

	movl    %esi, %edi      # Set up 1:1 and kernelspace mappings
	movl    $(PHYSMAP>>SUPERSIZE), %ecx
	movl    $(PERMS_KERNELSPACE),  %eax

1:	movl    %eax, (%edi)
	movl    %eax, (4*(KERNEL_SPACE>>SUPERSIZE))(%edi)
	addl    $4, %edi        # move to next page dir slots
	addl    $(4<<20), %eax  # entry for next superpage to be mapped
	decl    %ecx
	jnz     1b

	#------------------------------------------------------------------
	# Turn on paging/protected mode execution:

	movl    %esi, %cr3              # Set page directory

	mov     %cr4, %eax              # Enable super pages (CR4 bit 4)
	orl     $(1<<4), %eax
	movl    %eax, %cr4

	movl    %cr0, %eax              # Turn on paging (1<<31)
	orl     $((1<<31)|(1<<0)), %eax # and protection (1<<0)
	movl    %eax, %cr0

	movl    $high, %eax             # Make jump into kernel space
	jmp     *%eax
high:					# Now running at high addresses
	leal	stack, %esp		# Set up initial kernel stack

	call	initGDT			# Set up global segment table
	call	initIDT			# Set up interrupt descriptor table
	call	initPIC			# Set up PIC
	call	kernel			# Enter main kernel

	.globl	halt
halt:	hlt				# Catch all, in case kernel returns
	jmp	halt

#--------------------------------------------------------------------------
# Task-state Segment (TSS):
#
# We provide only a single Task-State Segment (TSS); we want to support
# lighter-weight task switching than is provided by the hardware.  But
# we still need a tss to store the kernel stack pointer and segment.
#--------------------------------------------------------------------------

	.data
tss:	.short	0, RESERVED		# previous task link
esp0:	.long	0			# esp0
	.short	KERN_DS, RESERVED	# ss0
	.long	0			# esp1
	.short	0, RESERVED		# ss1
	.long	0			# esp2
	.short	0, RESERVED		# ss2
	.long	0, 0, 0			# cr3 (pdbr), eip, eflags
	.long	0, 0, 0, 0, 0		# eax, ecx, edx, ebx, esp
	.long	0, 0, 0			# ebp, esi, edi
	.short	0, RESERVED		# es
	.short	0, RESERVED		# cs
	.short	0, RESERVED		# ss
	.short	0, RESERVED		# ds
	.short	0, RESERVED		# fs
	.short	0, RESERVED		# gs
	.short	0, RESERVED		# ldt segment selector
	.short	0			# T bit
	#
	# For now, we set the I/O bitmap offset to a value beyond the limit
	# of the tss; following Intel documentation, this means that there
	# is no I/O permissions bitmap and all I/O instructions will
	# generate exceptions when CPL > IOPL.
	#
	.short	1000			# I/O bit map base address
	.set	tss_len, .-tss

#--------------------------------------------------------------------------
# Initialize gdt:
#
# There are eight entries in our GDT:
#   0  null		; null entry required by Intel architecture
#   1  reserved
#   2  reserved
#   3  tss
#   4  kernel code	; kernel segments
#   5  kernel data
#   6  user code	; user segments
#   7  user data
# For the purposes of caching, we will start the GDT at a 128 byte aligned
# address; older processors have 32 byte cache lines while newer ones have
# 128 bytes per cache line.  The inclusion of a reserved entry (1) in the
# GDT ensures that the four {kernel,user}{code,data} segments all fit in a
# single cache line, even on older machines.  (I got this idea after reading
# the O'Reilly book on the Linux Kernel, but I have no idea if it makes
# a significant difference in practice ...)
#--------------------------------------------------------------------------

	.set	GDT_ENTRIES, 8
	.set	GDT_SIZE, 8*GDT_ENTRIES	# 8 bytes for each descriptor

	.data
	.align  128
#	.globl	gdt			# retain for debugging
gdt:	.space	GDT_SIZE, 0

	.align	8
gdtptr:	.short	GDT_SIZE-1
	.long	gdt

	.set	GDT_DATA,  0x13		# descriptor type for data segment
	.set	GDT_CODE,  0x1b		# descriptor type for code segment
	.set	GDT_TSS32, 0x09		# descriptor type for 32-bit tss

	.text
	.macro	gdtset name, slot, base, limit, gran, dpl, type
	#
	# This macro calculates a GDT segment descriptor from a specified
	# base address (32 bits), limit (20 bits), granularity (1 bit),
	# dpl (2 bits) and type (5 bits).  The descriptor is a 64 bit
	# quantity that is calculated in the register pair edx:eax and
	# also stored in the specified slot of the gdt.  The ebx and ecx
	# registers are also overwritten in the process.
	#
	# The format of a segment descriptor requires us to chop up the
	# base and limit values with bit twiddling manipulations that
	# cannot, in general, be performed at assembly time.  (The
	# base address, in particular, may be a relocatable symbol.)
	# The following macro makes it easier for us to perform the
	# necessary calculations for each segment at runtime.
	#
	# gran = 0 => limit is last valid byte offset in segment
	# gran = 1 => limit is last valid page offset in segment
	#
	# type = 0x13 (GDT_DATA)  => data segment
	# type = 0x1b (GDT_CODE)  => code segment
	# type = 0x09 (GDT_TSS32) => 32 bit tss system descriptor
	#
	# The following comments use # for concatenation of bitdata
	#
	.set	\name, (\slot<<3)|\dpl
	.globl	\name
	movl	$\base, %eax	# eax = bhi # bmd # blo
	movl	$\limit, %ebx	# ebx = ~ # lhi # llo

	mov	%eax, %edx	# edx = base
	shl	$16, %eax	# eax = blo # 0
	mov	%bx, %ax	# eax = blo # llo
	movl	%eax, gdt+(8*\slot)

	shr	$16, %edx	# edx = 0 # bhi # bmd
	mov	%edx, %ecx	# ecx = 0 # bhi # bmd
	andl	$0xff, %ecx	# ecx = 0 # 0   # bmd
	xorl	%ecx, %edx	# edx = 0 # bhi # bmd
	shl	$16,%edx	# edx = bhi # 0
	orl	%ecx, %edx	# edx = bhi # 0 # bmd
	andl	$0xf0000, %ebx	# ebx = 0 # lhi # 0
	orl	%ebx, %edx	# edx = bhi # 0 # lhi # 0 # bmd
	#
	# The constant 0x4080 used below is a combination of:
	#  0x4000     sets the D/B bit to indicate a 32-bit segment
	#  0x0080     sets the P bit to indicate that descriptor is present
	# (\gran<<15) puts the granularity bit into place
	# (\dpl<<5)   puts the protection level into place
	# \type       is the 5 bit type, including the S bit as its MSB
	#
	orl	$(((\gran<<15) | 0x4080 | (\dpl<<5) | \type)<<8), %edx
	movl	%edx, gdt + (4 + 8*\slot)
	.endm

initGDT:# Kernel code segment:
	gdtset	name=KERN_CS, slot=4, dpl=0, type=GDT_CODE, base=0, limit=0xffffff, gran=1

	# Kernel data segment:
	gdtset	name=KERN_DS, slot=5, dpl=0, type=GDT_DATA, base=0, limit=0xffffff, gran=1

	# User code segment
	gdtset	name=USER_CS, slot=6, dpl=3, type=GDT_CODE, base=0, limit=0xffffff, gran=1

	# User data segment
	gdtset	name=USER_DS, slot=7, dpl=3, type=GDT_DATA, base=0, limit=0xffffff, gran=1

	# TSS
	gdtset	name=TSS, slot=3, dpl=0, type=GDT_TSS32, base=tss, limit=tss_len-1, gran=0

	lgdt	gdtptr
	ljmp	$KERN_CS, $1f		# load code segment
1:
	mov	$KERN_DS, %ax		# load data segments
	mov 	%ax, %ds
	mov 	%ax, %es
	mov 	%ax, %ss
	mov	%ax, %gs
	mov 	%ax, %fs
	mov	$TSS, %ax		# load task register
	ltr	%ax
	ret

#--------------------------------------------------------------------------
# IDT:
#--------------------------------------------------------------------------

	.set	IDT_ENTRIES, 256	# Allow for all possible interrupts
	.set	IDT_SIZE, 8*IDT_ENTRIES	# Eight bytes for each idt descriptor
	.set	IDT_INTR, 0x000		# Type for interrupt gate
	.set	IDT_TRAP, 0x100		# Type for trap gate

	.data
	.align	8
idtptr:	.short	IDT_SIZE-1
	.long	idt
	.align  8
idt:	.space	IDT_SIZE, 0		# zero initial entries

	.text
	.macro	idtcalc	handler, slot, dpl=0, type=IDT_INTR, seg=KERN_CS
	#
	# This macro calculates an IDT segment descriptor from a specified
	# segment (16 bits), handler address (32 bits), dpl (2 bits) and
	# type (5 bits).  The descriptor is a 64 bit # quantity that is
	# calculated in the register pair edx:eax and then stored in the
	# specified slot of the IDT.
	#
	# type = 0x000 (IDT_INTR)  => interrupt gate
	# type = 0x100 (IDT_TRAP)  => trap gate
	#
	# The following comments use # for concatenation of bitdata
	#
	mov	$\seg, %ax		# eax =   ? # seg
	shl	$16, %eax		# eax = seg #   0
	movl	$\handler, %edx		# edx = hhi # hlo
	mov	%dx, %ax		# eax = seg # hlo
	mov	$(0x8e00 | (\dpl<<13) | \type), %dx
	movl	%eax, idt + (    8*\slot)
	movl	%edx, idt + (4 + 8*\slot)
	.endm

initIDT:# Add descriptors for exception & interrupt handlers:
	.irp	num, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,17,18,19
	idtcalc	exc\num, slot=\num
	.endr

	# Add descriptors for hardware irqs:
        .equ    IRQ_BASE,   0x20        # lowest hw irq number

        #intr    $IRQ_BASE, timerInterrupt

	# Add descriptors for system calls:
        # These are the only idt entries that we will allow to be called from
        # user mode without generating a general protection fault, so they
        # will be tagged with dpl=3
	idtcalc	handler=kputc, slot=0x80, dpl=3

	# Install the new IDT:
	lidt	idtptr
	ret

	#---------------------------------------------------------------------
	# Exception handlers:

	.text
	.macro	handler num, func, errorcode=0
	.align	16
exc\num:
	.if	\errorcode==0
	subl	$4, %esp	# fake an error code if necessary
	.endif
	push	%gs		# Save segments
	push	%fs
	push	%es
	push	%ds
	pusha			# Save registers
	push	%esp		# push pointer to frame for handler
	movl	$\num, %eax
	call	\func
	addl	$4, %esp
	popl	%es
	popl	%ds
	popa
	addl	$4, %esp	# remove error code
	iret
	.endm

	# Protected-mode exceptions and interrupts:
	#
	handler	num=0,  func=nohandler			# divide error
	handler	num=1,  func=nohandler			# debug
	handler	num=2,  func=nohandler			# NMI
	handler	num=3,  func=nohandler			# breakpoint
	handler	num=4,  func=nohandler			# overflow
	handler	num=5,  func=nohandler			# bound
	handler	num=6,  func=nohandler			# undefined opcode
	handler	num=7,  func=nohandler			# nomath
	handler	num=8,  func=nohandler, errorcode=1	# doublefault
	handler num=9,  func=nohandler			# coproc seg overrun
	handler num=10, func=nohandler, errorcode=1	# invalid tss
	handler num=11, func=nohandler, errorcode=1	# segment not present
	handler num=12, func=nohandler, errorcode=1	# stack-segment fault
	handler num=13, func=nohandler, errorcode=1	# general protection
	handler num=14, func=nohandler, errorcode=1	# page fault
	handler num=16, func=nohandler			# math fault
	handler num=17, func=nohandler, errorcode=1	# alignment check
	handler num=18, func=nohandler			# machine check
	handler num=19, func=nohandler			# SIMD fp exception

nohandler:			# dummy interrupt handler
#	leal	(initdir-KERNEL_SPACE), %ecx
#	movl	%ecx, %cr3	# reset page directory
	movl	4(%esp), %ebx	# get frame pointer
	pushl	%ebx
	pushl	%eax
	pushl	$excepted
	call	printf
	addl	$12, %esp

1:	hlt
	jmp 1b

	ret
excepted:
	.asciz	"Exception 0x%x, frame=0x%x\n"

pfhandler:
	leal	(initdir-KERNEL_SPACE), %ecx
	movl	%ecx, %cr3	# reset page directory
	movl	4(%esp), %ebx	# get frame pointer
	movl	48(%ebx), %eax  # get error code from frame
        pushl	%eax
	movl	%cr2, %eax	# get fault code
	pushl	%eax
        movl	52(%ebx), %eax	# get faulting eip
        pushl	%eax
	pushl	$pfmsg
	call	printf
	addl	$16, %esp
	jmp	halt

pfmsg:
	.asciz	"Page Fault at eip=0x%x, Fault addr=0x%x, Error=0x%x\n"

	#------------------------------------------------------------------
	# Initialize PIC:
	# Configure standard 8259 programmable interrupt controller
	# to remap hardware irqs 0x0-0xf into the range 0x20-0x2f.
	.equ    PIC_1, 0x20
	.equ    PIC_2, 0xa0

	# Send ICWs (initialization control words) to initialize PIC.
	# NOTE: Some sources suggest that there should be a delay between
	# each output byte ... but I don't see that in the datasheet ...
	.macro  initpic port, base, info, init
	movb    $0x11, %al
	outb    %al, $\port     # ICW1: Initialize + will be sending ICW4

	movb    $\base, %al     # ICW2: Interrupt vector offset
	outb    %al, $(\port+1)

	movb    $\info, %al     # ICW3: Configure for two PICs
	outb    %al, $(\port+1)

	movb    $0x01, %al      # ICW4: 8086 mode
	outb    %al, $(\port+1)

	movb    $\init, %al     # OCW1: set initial mask
	outb    %al, $(\port+1)
	.endm

initPIC:initpic PIC_1, IRQ_BASE,   0x04, 0xfb  # all but IRQ2 masked out
	initpic PIC_2, IRQ_BASE+8, 0x02, 0xff
	ret

#--------------------------------------------------------------------------
# Simple system call handler:
#--------------------------------------------------------------------------

	.text
kputc:	subl	$4, %esp	# Fake an error code
	push	%gs		# Save segments
	push	%fs
	push	%es
	push	%ds
	pusha			# Save registers
	leal	stack, %esp	# Switch to kernel stack
	jmp	kputc_imp

#--------------------------------------------------------------------------
# Switch to user mode:  Takes a single parameter, which provides the
# initial context for the user process.
#
# Size of context is 4 * (8 + 4 + 6) = 4*18 = 72
# - 8 general registers: edi, esi, ebp, esp, ebx, edx, ecx, eax
# - 4 segment registers: ds, es, fs, gs
# - 6 interrupt frame words: errorcode, eip, cs, eflags, esp, ss
#--------------------------------------------------------------------------

	.set	CONTEXT_SIZE, 72
	.globl	switchToUser
switchToUser:
	movl	4(%esp), %eax	# Load address of the user context
	movl	%eax, %esp	# Reset stack to base of user context
	addl	$CONTEXT_SIZE, %eax
	movl	%eax, esp0	# Set stack address for kernel reentry
	popa			# Restore registers
	pop	%ds		# Restore segments
	pop	%es
	pop	%fs
	pop	%gs
	addl	$4, %esp	# Skip error code
	iret			# Return from interrupt

#-- Done ---------------------------------------------------------------------

#--------------------------------------------------------------------------
# Boot code: machine initialization and low-level primitives
# Mark P Jones, Portland State University
#--------------------------------------------------------------------------
#include "kip.h"

	.global entry
entry:	cli				# Turn off interrupts

        #------------------------------------------------------------------
        # Create initial page directory in which the initial PHYSMAP
        # portion of physical memory is mapped 1:1 and into KERNEL_SPACE.

	# Address of page dir:  (we're not in high memory yet ...)
	leal	(pdir-KERNEL_SPACE), %edi
	movl	%edi, %esi	# save in %esi

        movl    $1024, %ecx	# Zero out complete page directory
	movl	$0, %eax
1:      movl	%eax, (%edi)
        addl    $4, %edi
        decl    %ecx
        jnz     1b

        movl    %esi, %edi      # Set up 1:1 and kernelspace mappings
	movl	$(PHYSMAP>>SUPERSIZE), %ecx
        movl    $(PERMS_KERNELSPACE),  %eax 

1:	movl	%eax, (%edi)
	movl	%eax, (4*(KERNEL_SPACE>>SUPERSIZE))(%edi)
	addl	$4, %edi	# move to next page dir slots
	addl	$(4<<20), %eax	# entry for next superpage to be mapped
	decl	%ecx
	jnz	1b

        #------------------------------------------------------------------
        # Turn on paging/protected mode execution:

        movl    %esi, %cr3		# Set page directory

        mov     %cr4, %eax              # Enable super pages (CR4 bit 4)
        orl     $((1<<4)|(1<<8)), %eax	# and rdpmc (CR4 bit 8)
        movl    %eax, %cr4

        movl    %cr0, %eax              # Turn on paging (1<<31)
        orl     $((1<<31)|(1<<0)), %eax # and protection (1<<0)
        movl    %eax, %cr0

	movl	$high, %eax		# Make jump into kernel space
	jmp	*%eax
high:					# Now running at high addresses
        leal    kernelstack, %esp       # Set up initial kernel stack

	#------------------------------------------------------------------
	# Initialize GDT:
	#
	# There are eight entries in our GDT:
	#   0  null		; null entry required by Intel architecture
	#   1  reserved
	#   2  utcb
	#   3  tss
	#   4  kernel code	; kernel segments
        #   5  kernel data
        #   6  user code	; user segments
        #   7  user data
	# For the purposes of caching, we will start the GDT at a 128 byte
	# aligned address; older processors have 32 byte cache lines while
	# newer ones may have 128 bytes per cache line.  The inclusion of
	# a reserved entry (1) in the GDT ensures that the four
	# {kernel,user}{code,data} segments all fit in a single cache line,
	# even on older machines.  (I got this idea after reading the
	# O'Reilly book on the Linux Kernel ...)

	.equ	GDT_ENTRIES, 8		# Eight entries, as described above

	.equ	GDT_DATA,  0x13		# Descriptor type for data segment
	.equ	GDT_CODE,  0x1b		# Descriptor type for code segment
	.equ	GDT_TSS32, 0x09		# Descriptor type for 32-bit tss

	# The gdtset macro calculates a GDT segment descriptor from a
	# specified base address (32 bits), limit (20 bits), granularity
        # (1 bit), dpl (2 bits) and type (5 bits).  The descriptor is
        # a 64 bit quantity that is calculated in the register pair
        # edx:eax and also stored in the specified slot of the gdt.
	# The ebx and ecx registers are also overwritten in the process.
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

	.macro	gdtset slot, name, base, limit, gran, dpl, type
	.equ	\name, (\slot<<3)|\dpl
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
	orl	%ebx, %edx	# eax = bhi # 0 # lhi # 0 # bmd

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

	# Generate GDT entries: -------------------------------------------
	.global USER_CS, USER_DS, KERN_CS, KERN_DS, UTCB_DS

	# Kernel code segment:
	gdtset	slot=4, name=KERN_CS, dpl=0, type=GDT_CODE, \
		base=0, limit=0xffffff, gran=1

	# Kernel data segment:
	gdtset	slot=5, name=KERN_DS, dpl=0, type=GDT_DATA, \
		base=0, limit=0xffffff, gran=1

	# User code segment
	gdtset	slot=6, name=USER_CS, dpl=3, type=GDT_CODE, \
		base=0, limit=0xffffff, gran=1

	# User data segment
	gdtset	slot=7, name=USER_DS, dpl=3, type=GDT_DATA, \
		base=0, limit=0xffffff, gran=1

	# TSS
	gdtset	slot=3, name=TSS, dpl=0, type=GDT_TSS32, \
		base=tss, limit=tss_len-1, gran=0

	# UTCB pointer segment
	gdtset	slot=2, name=UTCB_DS, dpl=3, type=GDT_DATA, \
		base=UTCBPTR, limit=4, gran=0

	lgdt	gdtptr
	ljmp	$KERN_CS, $1f		# load code segment
1:
	mov	$KERN_DS, %ax		# load data segments
	mov 	%ax, %ds
	mov 	%ax, %es
	mov 	%ax, %ss
	mov 	$UTCB_DS, %ax		# load utcb segment
	mov	%ax, %gs
	mov	$0, %ax			# load unused segments
	mov 	%ax, %fs
	mov	$TSS, %ax		# load task register
	ltr	%ax
	movl	$kernelstack, esp0	# set default kernel stack

	#------------------------------------------------------------------
	# Initialize IDT:
	#
	# This macro calculates an IDT segment descriptor from a specified
	# segment (16 bits), handler address (32 bits), dpl (2 bits) and
	# type (5 bits).  The descriptor is a 64 bit # quantity that is
	# calculated in the register pair edx:eax and then stored in the
	# specified IDT slot.

	.equ	IDT_INTR, 0x000		# Type for interrupt gate
	.equ	IDT_TRAP, 0x100		# Type for trap gate

	.macro	idtcalc	slot, handler, dpl=0, type=IDT_INTR, seg=KERN_CS
	# The following comments use # for concatenation of bitdata
	mov	$\seg, %ax		# eax =   ? # seg
	shl	$16, %eax		# eax = seg #   0
	movl	$\handler, %edx		# edx = hhi # hlo
	mov	%dx, %ax		# eax = seg # hlo
	movl	%eax, idt+(8*\slot)
	mov	$(0x8e00 | (\dpl<<13) | \type), %dx
	movl	%edx, idt + (4 + 8*\slot)
	.endm

        # The following macro fills in an idt slot and creates a wrapper
        # for an exception/interrupt handler.  Some (but not all) types of
	# exception pass an error code argument on the stack.  For a more
	# uniform treatment, we include an error code argument for every
	# exception, faking it when there is no suitable value to be passed.

	.equ	NOERR, (~0)	# No error code is produced; need a fake
	.equ	HWERR, (~1)	# Hardware produces an error code

	.macro intr slot, service, err=NOERR, dpl=0, type=IDT_INTR, seg=KERN_CS
	idtcalc \slot, handle\slot, \dpl, \type, \seg
	.section .handlers
	.align	16
handle\slot:
	.if	(\err)==NOERR
	subl	$4, %esp		# Fake an error code
	.else
	.if 	(\err)!=HWERR		# Or push a specific error code
	pushl	$\err
	.endif
	.endif
        push	%gs      		# Save segment registers
        push	%fs
        push	%es
        push	%ds
        pusha				# Save other user registers
        leal	kernelstack, %esp	# Switch to kernel stack
	jmp	\service
	.text
	.endm

        # Descriptors and handlers for exceptions: ------------------------
        intr	0, divideError
        intr	1, debug
        intr	2, nmiInterrupt
        intr	3, breakpoint
        intr	4, overflow

        intr	5, boundRangeExceeded
        intr	6, invalidOpcode
        intr	7, deviceNotAvailable
        intr	8, doubleFault,         err=HWERR
        intr	9, coprocessorSegmentOverrun

        intr	10, invalidTSS,         err=HWERR
        intr	11, segmentNotPresent,  err=HWERR
        intr	12, stackSegmentFault,  err=HWERR
        intr	13, generalProtection,  err=HWERR
        intr	14, pageFault,          err=HWERR

        // Slot 15 is Intel Reserved
        intr	16, floatingPointError
        intr	17, alignmentCheck,     err=HWERR
        intr	18, machineCheck
        intr	19, simdFloatingPointException

        // Slots 20-31 are Intel Reserved

	# Add descriptors for hardware irqs: ------------------------------
	.equ	IRQ_BASE,   0x20	# lowest hw irq number

	# This is ugly, but it's the best I could come up with short of
	# some way of forcing macro arguments to be passed by value and
	# not by text.  A remaining problem is that there is no direct
	# connection between the list of IRQ numbers and IRQ_BASE.
	.irp	num,      0x21,0x22,0x23, 0x24,0x25,0x26,0x27, \
		     0x28,0x29,0x2a,0x2b, 0x2c,0x2d,0x2e,0x2f
	intr	\num, service=hardwareIRQ, err=(\num-IRQ_BASE)
	.endr

	intr	0x20, timerInterrupt
	# TODO: Should we do anything special for IRQ2?

	# Add descriptors for system calls: -------------------------------
        # These are the only idt entries that we will allow to be called
        # from user mode without generating a general protection fault,
	# so they will be tagged with dpl=3.
	intr	INT_THREADCONTROL, threadControl,     err=NOERR, dpl=3
	intr	INT_SPACECONTROL,  spaceControl,      err=NOERR, dpl=3
	intr	INT_IPC,           ipc,               err=NOERR, dpl=3
	intr	INT_EXCHANGEREGS,  exchangeRegisters, err=NOERR, dpl=3
	intr	INT_SCHEDULE,      schedule,          err=NOERR, dpl=3
	intr	INT_THREADSWITCH,  threadSwitch,      err=NOERR, dpl=3
	intr	INT_UNMAP,         unmap,             err=NOERR, dpl=3
	intr	INT_PROCCONTROL,   processorControl,  err=NOERR, dpl=3
	intr	INT_MEMCONTROL,    memoryControl,     err=NOERR, dpl=3
	intr	INT_SYSTEMCLOCK,   systemClock,       err=NOERR, dpl=3

	lidt	idtptr		# Install the new IDT

	#------------------------------------------------------------------
	# Initialize PIC:
	# Configure standard 8259 programmable interrupt controller
	# to remap hardware irqs 0x0-0xf into the range 0x20-0x2f.
	.equ	PIC_1, 0x20
	.equ	PIC_2, 0xa0

	# Send ICWs (initialization control words) to initialize PIC.
	# TODO: Some sources suggest that there should be a delay between
	# each output byte ... but I don't see that in the datasheet ...
	.macro	initpic port, base, info, init
	movb	$0x11, %al
	outb	%al, $\port	# ICW1: Initialize + will be sending ICW4

	movb	$\base, %al	# ICW2: Interrupt vector offset 
	outb	%al, $(\port+1)

	movb	$\info, %al	# ICW3: Configure for two PICs
	outb	%al, $(\port+1)

	movb	$0x01, %al	# ICW4: 8086 mode
	outb	%al, $(\port+1)

	movb	$\init, %al	# OCW1: set initial mask
	outb	%al, $(\port+1)
	.endm

	initpic	PIC_1, IRQ_BASE,   0x04, 0xfb  # all but IRQ2 masked out
	initpic	PIC_2, IRQ_BASE+8, 0x02, 0xff

        jmp    init		# Jump off into kernel, no return!

	#------------------------------------------------------------------
	# Halt processor: Also used as code for the idle thread.
	.global halt
halt:	hlt
	jmp	halt

	#------------------------------------------------------------------
	# Data areas:
	.data
	.align	(1<<PAGESIZE)
	.global	kernelstack
        .space  4096
kernelstack:				# Kernel stack

	.align	(1<<PAGESIZE)
pdir:	.space	4096   			# Initial page directory

	.align  128
	.equ	GDT_SIZE, 8*GDT_ENTRIES	# 8 bytes for each descriptor
gdt:	.space	GDT_SIZE, 0		# Global descriptor table (GDT)

	.align	8
	.short  0
gdtptr:	.short	GDT_SIZE-1		# GDT pointer
	.long	gdt

	.align  128
	.equ	IDT_ENTRIES, 256	# Allow for all possible interrupts
	.equ	IDT_SIZE, 8*IDT_ENTRIES	# Eight bytes for each idt descriptor
idt:	.space	IDT_SIZE, 0		# Zero initial entries

	.align	8
	.short  0
idtptr:	.short	IDT_SIZE-1
	.long	idt

	#------------------------------------------------------------------
	# Task-state Segment (TSS): We provide only a single Task-State
	# Segment (TSS); we want to support lighter-weight task switching
	# than is provided by the hardware.  But we still need a tss to
	# store the kernel stack pointer and segment.
	.align  128
tss:	.short	0, RESERVED		# previous task link
	.global	esp0
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
	# For now, we set the I/O bitmap offset to a value beyond the
	# limit of the tss; following Intel documentation, this means
	# that there is no I/O permissions bitmap and all I/O instructions
	# will generate exceptions when CPL > IOPL.
	.short	1000			# I/O bit map base address
	.equ	tss_len, .-tss

#--------------------------------------------------------------------------

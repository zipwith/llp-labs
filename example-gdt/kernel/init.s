#--------------------------------------------------------------------------
# init.s:  Initialize simple kernel with a GDT
#
# Mark P. Jones, April 2006, 2016

#--------------------------------------------------------------------------
# General definitions:
#--------------------------------------------------------------------------

	.set	RESERVED, 0	# Used to mark a reserved field

#--------------------------------------------------------------------------
# Initial stack:
#--------------------------------------------------------------------------

	.data
	.space	4096		# Kernel stack
stack:

#--------------------------------------------------------------------------
# Entry point:
#--------------------------------------------------------------------------

	.text
	.globl	entry
entry:	cli			# Turn off interrupts
	leal	stack, %esp	# Set up initial kernel stack

	call	initGDT		# Set up global segment table
	call	kernel		# Enter main kernel

1:	hlt			# Catch all, in case kernel returns
	jmp	1b

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

#-- Done ---------------------------------------------------------------------

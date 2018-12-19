#--------------------------------------------------------------------------
# userlib.s:  User-level initialization and system call stubs.
#
# Mark P. Jones, Portland State University, May 2016

#--------------------------------------------------------------------------
# Initialization:
#--------------------------------------------------------------------------

	.text
	.globl	entry
entry:	leal	stack, %esp
	jmp	cmain

	.data
	.space	4096		# User stack
stack:

#--------------------------------------------------------------------------
# Request kernel dump of current state (for debugging):
#--------------------------------------------------------------------------

	# System call to request a kernel dump of the invoking
	# thread's page directory, cspace, and system wide untypeds.
	.globl	dump
dump:	int	$101
	ret

#--------------------------------------------------------------------------
# Request kernel mapping without the use of a capability:
#--------------------------------------------------------------------------

	# System call to request a page of memory to be mapped
	# at a specific address.
	#    kmapPage(addr)
	#    | retn | addr |
	#    | 0    | 4    |
	.globl	kmapPage
kmapPage:
	movl	4(%esp), %esi
	int	$100
	ret

#--------------------------------------------------------------------------
# Console access:
#--------------------------------------------------------------------------

	# System call to print a character in the kernel's window
	#    kputc(cap, ch)
	#    | retn | cap  | ch   |
	#    | 0    | 4    | 8    |
	.globl	kputc
kputc:	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	int	$128
	ret

#--------------------------------------------------------------------------
# Capability space management:
#--------------------------------------------------------------------------

	# System call to move a capability
	#    capmove(src, dst, copy)
	#    | retn | src  | dst  | copy |
	#    | 0    | 4    | 8    | 12   |
	.globl	capmove
capmove:movl	4(%esp), %esi
	movl	8(%esp), %edi
	movl	12(%esp), %eax
	int	$140
	ret

	# System call to clear a capability
	#    capclear(src, dst)
	#    | retn | src  |
	#    | 0    | 4    |
	.globl	capclear
capclear:
	movl	4(%esp), %esi
	int	$141
	ret

#--------------------------------------------------------------------------
# Allocation from untyped memory:
#--------------------------------------------------------------------------

	# System call to allocate a new untyped memory object
	#    allocUntyped(ucap, slot, bits)
	#    | retn | ucap | slot | bits |
	#    |      | ecx  | edi  | eax  |
	#    | 0    | 4    | 8    | 12   |
	.globl	allocUntyped
allocUntyped:
	movl	4(%esp), %ecx
	movl	8(%esp), %edi
	movl	12(%esp), %eax
	int	$142
	ret

	# System call to allocate a new cspace object:
	#    allocCspace(ucap, slot)
	#    | retn | ucap | slot |
	#    |      | ecx  | edi  |
	#    | 0    | 4    | 8    |
	.globl	allocCspace
allocCspace:
	movl	4(%esp), %ecx
	movl	8(%esp), %edi
	int	$143
	ret

	# System call to allocate a new page object:
	#    allocPage(ucap, slot)
	#    | retn | ucap | slot |
	#    |      | ecx  | edi  |
	#    | 0    | 4    | 8    |
	.globl	allocPage
allocPage:
	movl	4(%esp), %ecx
	movl	8(%esp), %edi
	int	$144
	ret

	# System call to allocate a new page table:
	#    allocPageTable(ucap, slot)
	#    | retn | ucap | slot |
	#    |      | ecx  | edi  |
	#    | 0    | 4    | 8    |
	.globl	allocPageTable
allocPageTable:
	movl	4(%esp), %ecx
	movl	8(%esp), %edi
	int	$145
	ret

#--------------------------------------------------------------------------
# Mapping into the current address space:
#--------------------------------------------------------------------------

	# System call to map a page:
	#    mapPageTable(pcap, addr)
	#    | retn | pcap | addr |
	#    |      | ecx  | eax  |
	#    | 0    | 4    | 8    |
	.globl	mapPage
mapPage:
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	int	$150
	ret

	# System call to allocate a new page table:
	#    mapPageTable(ptcap, addr)
	#    | retn | ptcap| addr |
	#    |      | ecx  | eax  |
	#    | 0    | 4    | 8    |
	.globl	mapPageTable
mapPageTable:
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	int	$151
	ret

#-- Done ---------------------------------------------------------------------

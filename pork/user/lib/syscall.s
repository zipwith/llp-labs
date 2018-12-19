        .equ    INT_UNMAP,         0x76
        .equ    INT_PROCCONTROL,   0x77
        .equ    INT_MEMCONTROL,    0x78


	.text
	.global	readTSC
	# -----------------------------------------------------------------
	# unsigned readTSC(unsigned* hi)
readTSC:
	rdtsc
	movl	4(%esp), %ecx
	movl	%edx, (%ecx)
	ret

	# -----------------------------------------------------------------
	# void* L4_KernelInterface	// On entry:
	#  (L4_Word_t* apiVersion,	//  4(%esp)
	#   L4_Word_t* apiFlags,	//  8(%esp)
	#   L4_Word_t* kernelId)	// 12(%esp)

	.global L4_KernelInterface
L4_KernelInterface:
	lock				# Trigger kernel interface syscall
	nop
	pushl	%eax			# Save pointer to kip base
	movl	8(%esp), %eax		# Save api version from ecx
	movl	%ecx, (%eax)
	movl	12(%esp), %eax		# Save api flags from edx
	movl	%edx, (%eax)
	movl	16(%esp), %eax		# Save kernel id from edx
	movl	%edx, (%eax)
	popl	%eax			# Recover KIP base pointer
	ret

	# -----------------------------------------------------------------
	# L4_Word_t L4_ThreadControl	// On entry:
        #  // esi, edi, return addr     // -- 12 bytes
	#  (L4_ThreadId_t dest,		// 12(%esp)
	#   L4_ThreadId_t spaceSpec,	// 16(%esp)
	#   L4_ThreadId_t scheduler,	// 20(%esp)
	#   L4_ThreadId_t pager,	// 24(%esp)
	#   void* utcbLocation)		// 28(%esp)

	.global L4_ThreadControl
L4_ThreadControl:
        pushl	%esi
	pushl	%edi

	movl	12(%esp), %eax		# dest
	movl	16(%esp), %esi		# space specifier
	movl	20(%esp), %edx		# scheduler
	movl	24(%esp), %ecx		# pager
	movl	28(%esp), %edi		# utcb location

	int	$0x70			# TODO: should lookup in KIP ...

	popl	%edi
	popl	%esi
	ret

	# -----------------------------------------------------------------
	# (use Prim version to avoid returning a structure type)
	# L4_Word_t L4_Prim_ExchangeRegisters	// On entry:
	#  // edi, esi, ebx, ebp, return addr   // -- 20 bytes
	#  (L4_ThreadId_t dest,			// 20(%esp)
	#   L4_Word_t     control,		// 24(%esp)
	#   L4_Word_t     sp,			// 28(%esp)
	#   L4_Word_t     ip,			// 32(%esp)
	#   L4_Word_t     flags,		// 36(%esp)
	#   L4_Word_t     userDefinedHandle,	// 40(%esp)
	#   L4_Word_t     pager,		// 44(%esp)
	#   L4_Word_t*    oldControl,		// 48(%esp)
	#   L4_Word_t*    oldSp,		// 52(%esp)
	#   L4_Word_t*    oldIp,		// 56(%esp)
	#   L4_Word_t*    oldFlags,		// 60(%esp)
	#   L4_Word_t*    oldUserDefinedHandle,	// 64(%esp)
	#   L4_ThreadId_t* oldPager)		// 68(%esp)

	.global	L4_Prim_ExchangeRegisters
L4_Prim_ExchangeRegisters:
	pushl	%ebp			# Save calling function context
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	20(%esp),  %eax		# dest
	movl	24(%esp),  %ecx		# control
	movl	28(%esp), %edx		# sp
	movl	32(%esp), %esi		# ip
	movl	36(%esp), %edi		# flags
	movl	40(%esp), %ebx		# userDefinedHandle
	movl	44(%esp), %ebp		# pager

	int	$0x73			# TODO: should indirect via KIP ...

	pushl	%eax			# Push result, add 4 bytes to offsets
	movl	52(%esp), %eax		# Save old control
	movl	%ecx, (%eax)
	movl	56(%esp), %eax		# Save old sp
	movl	%edx, (%eax)
	movl	60(%esp), %eax		# Save old ip
	movl	%esi, (%eax)
	movl	64(%esp), %eax		# Save old flags
	movl	%edi, (%eax)
	movl	68(%esp), %eax		# Save old userDefinedHandle
	movl	%ebx, (%eax)
	movl	72(%esp), %eax		# Save old pager
	movl	%ebp, (%eax)
	popl	%eax			# Pop result	

	popl	%edi			# Restore calling function context
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret				# result is in %eax

	# -----------------------------------------------------------------
	# void L4_Start_SpIpFlags	        // On entry:
	#  // edi, esi, ebx, ebp, return addr	// -- 20 bytes
	#  (L4_ThreadId_t dest,			// 20(%esp)
	#   L4_Word_t     sp,			// 24(%esp)
	#   L4_Word_t     ip,			// 28(%esp)
	#   L4_Word_t     flags)		// 32(%esp)

	.global	L4_Start_SpIpFlags
L4_Start_SpIpFlags:

	pushl	%ebp			# Save calling function context
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	20(%esp), %eax		# dest
	movl	$((1<<8)|(7<<3)|6),%ecx	# control: (h | fis | SR)
	movl	24(%esp), %edx		# sp
	movl	28(%esp), %esi		# ip
	movl	32(%esp), %edi		# flags

	int	$0x73			# TODO: should indirect via KIP ...

	popl	%edi			# Restore calling function context
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

	# -----------------------------------------------------------------
	# void L4_ThreadSwitch		// On entry:
	#  (L4_ThreadId_t dest)		// 4(%esp)

	.global L4_ThreadSwitch
L4_ThreadSwitch:
	movl	4(%esp), %eax		# dest
	int	$0x75			# TODO: should indirect via KIP ...
	ret				# No result

	# -----------------------------------------------------------------
	# L4_Word_t L4_Schedule		// On entry:
	#  // esi, edi, return addr	// -- 12 bytes
	#  (L4_ThreadId_t dest,		// 12(%esp)
	#   L4_Word_t tsLen,		// 16(%esp)
	#   L4_Word_t totQuantum,	// 20(%esp)
	#   L4_Word_t procControl,	// 24(%esp)
	#   L4_Word_t prio,		// 28(%esp)
	#   L4_Word_t* remTimeslice,	// 32(%esp)
	#   L4_Word_t* remQuantum)	// 36(%esp)

	.global L4_Schedule
L4_Schedule:
	pushl	%esi
	pushl	%edi

	movl	12(%esp), %eax		# dest
	movl	16(%esp), %ecx		# tsLen
	movl	20(%esp), %edx		# totQuantum
	movl	24(%esp), %esi		# procControl
	movl	28(%esp), %edi		# prio
	int	$0x74			# TODO: should indirect via KIP ...
	movl	32(%esp), %esi		# save remTimeslice from ecx
	movl	%ecx, (%esi)
	movl	36(%esp), %esi		# save remQuantum from edx
	movl	%edx, (%esi)

	popl	%edi
	popl	%esi
	ret				# result is in %eax

	# -----------------------------------------------------------------
	# L4_Word_t L4_SpaceControl	// On entry:	eax->
	#  // esi, return addr          // -- 8 bytes
	#  (L4_ThreadId_t spaceSpec,	//  8(%esp)	->eax
	#   L4_Word_t control,		// 12(%esp)	->ecx
	#   L4_Fpage_t kipArea,		// 16(%esp)	->edx
	#   L4_Fpage_t utcbArea,	// 20(%esp)	->esi
	#   L4_Word_t* oldControl)	// 24(%esp)	ecx->

	.global	L4_SpaceControl
L4_SpaceControl:
	pushl	%esi

	movl	8(%esp), %eax		# spaceSpec
	movl	12(%esp), %ecx		# control
	movl	16(%esp), %edx		# kipArea
	movl	20(%esp), %esi		# utcbArea
	int	$0x71			# TODO: should indirect via KIP ...
	movl	24(%esp), %esi		# save oldControl from ecx
	movl	%ecx, (%esi)

	popl	%esi
	ret				# result is in %eax 

	# -----------------------------------------------------------------
	# (use Prim version to avoid returning a structure type)
	# L4_Word_t L4_Prim_Ipc			// On entry:	mr0->
	#  // edi, esi, ebx, ebp, return addr   // -- 20 bytes
	#  (L4_ThreadId_t to,			// 20(%esp)	-->eax
	#   L4_ThreadId_t fromSpec,		// 24(%esp)	-->edx
	#   L4_ThreadId_t* from)		// 28(%esp)	eax->

	.global	L4_Prim_Ipc
L4_Prim_Ipc:
	pushl	%ebp			# Save calling function context
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	20(%esp), %eax		# to
	movl	24(%esp), %edx		# fromSpec
	movl	%gs:0,    %edi		# UTCB
	movl	(%edi),   %esi		# MR0

	int	$0x72			# TODO: should indirect via KIP ...

	# TODO: This code needs careful review to check correct use of
	# registers, etc...  e.g., what happens to MR1, MR2?
	movl	28(%esp), %ebx		# save "from" from eax
	movl	%eax, (%ebx)
	movl	(%edi), %eax		# Grab MR0 in eax for return

#	movl	%esi, %eax		# return with MR0 in eax
#	movl	%eax, (%edi)		# and save MR0 in UTCB

	popl	%edi			# Restore calling function context
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret

	# -----------------------------------------------------------------
	# TODO: it might be better to store the system clock in the kip so
	# that it can be accessed from user space without a system call.
	# (on the other hand: does this raise synchronization issues?)
	# L4_Word64_t L4_SystemClock()

	.global L4_Prim_SystemClock
L4_Prim_SystemClock:
	int	$0x79			# TODO: should indirect via KIP ...
	ret				# Result in edx:eax


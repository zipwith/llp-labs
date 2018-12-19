#--------------------------------------------------------------------------
# Kernel Information Page:
# Mark P Jones, Portland State University
#--------------------------------------------------------------------------
#include "kip.h"

		.data
		.align	(1<<PAGESIZE)
		.global	Kip, KipEnd
Kip:		.byte	'L', '4', 230, 'K'
		.long	API_VERSION,  API_FLAGS, (KernelDesc - Kip)

		.global	Sigma0Server, Sigma1Server, RootServer
Kdebug:		.long	0, 0, 0, 0		# Kernel debugger information
Sigma0Server:	.long	0, 0, 0, 0		# Sigma0 information
Sigma1Server:	.long	0, 0, 0, 0		# Sigma1 information
RootServer:	.long	0, 0, 0, 0		# Rootserver information
		.long	RESERVED

		.global	MemoryInfo
		.macro memoryInfo offset, number
		.long	((\offset<<16) | \number)
		.endm
MemoryInfo:	 memoryInfo offset=(MemDesc-Kip), number=0

KdebugConfig:	.long	0, 0

		.long	RESERVED, RESERVED, RESERVED, RESERVED
		.long	RESERVED, RESERVED, RESERVED, RESERVED
		.long	RESERVED, RESERVED, RESERVED, RESERVED
		.long	RESERVED, RESERVED, RESERVED, RESERVED
		.long	RESERVED

VirtRegInfo:	.long	NUMMRS-1		# virtual register information

		.macro	utcbInfo size, align, multiplier
		.long	(((\size<<6)|\align)<<10)|\multiplier
		.endm
UtcbInfo:	utcbInfo size=MIN_UTCBAREASIZE, align=UTCBSIZE, multiplier=1

KipAreaInfo:	.long	KIPAREASIZE		# Kip area information

		.long	RESERVED, RESERVED

BootInfo:	.long	0			# Boot information

ProcDescPtr:	.long	(ProcDesc - Kip)	# Processor descriptor pointer

ClockInfo:	.long	0			# TODO: schedule precision

		.macro	threadInfo user, system, bits
		.long	((((\user<<12) | \system)<<8) | \bits)
		.endm
ThreadInfo:	threadInfo user=USERBASE, system=SYSTEMBASE, bits=THREADBITS

PageInfo:	.long	(1<<SUPERSIZE) | (1<<PAGESIZE) | R | W;

		.macro	processorInfo logProcDescSize, numProcessors
		.long	(\logProcDescSize<<28) | (\numProcessors-1)
		.endm
ProcessorInfo:	processorInfo logProcDescSize=3, numProcessors=1

SystemCalls:	.long	(spaceControlEntry      - Kip)
		.long	(threadControlEntry     - Kip)
		.long	(processorControlEntry  - Kip)
		.long	(memoryControlEntry     - Kip)
		.long	(ipcEntry               - Kip)
		.long	(lipcEntry              - Kip)
		.long	(unmapEntry             - Kip)
		.long	(exchangeRegistersEntry - Kip)
		.long	(systemClockEntry       - Kip)
		.long	(threadSwitchEntry      - Kip)
		.long	(scheduleEntry          - Kip)
		.long	RESERVED

		.global	MemDesc
MemDesc:	.space	8*MAX_MEMDESC		# Memory Descriptors

ProcDesc:	.long	0, 0			# Processor Descriptors

		.global	KernelDesc
KernelDesc:	.long	KERNEL_ID		# Kernel Descriptor

		.macro	kernelGenDate day, month, year
		.long	(\year-2000)<<9 | (\month<<5) | \day
		.endm
		kernelGenDate day=4, month=2, year=2007

		.macro	kernelVer ver, subver, subsubver
		.long	(((\ver<<8) | \subver)<<16) | \subsubver
		.endm
		kernelVer ver=1, subver=2, subsubver=0

		.ascii	"pork"

		.global	KernelBanner
KernelBanner:	.asciz	"The Portland L4 Kernel (pork), February 2007"
		.byte	0

		#-- Privileged system call entry points: ------------------
		.align	128
spaceControlEntry:
		int	$INT_SPACECONTROL
		ret

threadControlEntry:
		int	$INT_THREADCONTROL
		ret

processorControlEntry:
		int	$INT_PROCCONTROL
		ret

memoryControlEntry:
		int	$INT_MEMCONTROL
		ret

		#-- System call entry points: -----------------------------
ipcEntry:	int	$INT_IPC
		ret

lipcEntry:	int	$INT_IPC
		ret

unmapEntry:	int	$INT_UNMAP
		ret

exchangeRegistersEntry:
		int	$INT_EXCHANGEREGS
		ret

threadSwitchEntry:
		int	$INT_THREADSWITCH
		ret

scheduleEntry:	int	$INT_SCHEDULE
		ret

systemClockEntry:
		int	$INT_SYSTEMCLOCK
		ret
KipEnd:

#--------------------------------------------------------------------------

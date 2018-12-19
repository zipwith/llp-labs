/*
    Copyright 2014-2018 Mark P Jones, Portland State University

    This file is part of CEMLaBS/LLP Demos and Lab Exercises.

    CEMLaBS/LLP Demos and Lab Exercises is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    CEMLaBS/LLP Demos and Lab Exercises is distributed in the hope that
    it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CEMLaBS/LLP Demos and Lab Exercises.  If not, see
    <https://www.gnu.org/licenses/>.
*/
/*-------------------------------------------------------------------------
 * Contexts for user code:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef CONTEXT_H
#define CONTEXT_H

/*-------------------------------------------------------------------------
 * Context data structures:
 */
struct Registers {	// Stack frame created by pusha
  unsigned edi;
  unsigned esi;
  unsigned ebp;
  unsigned esp;		// TODO: could we use this for other storage?
  unsigned ebx;
  unsigned edx;
  unsigned ecx;
  unsigned eax;
};

struct Segments {	// A frame to hold user segment registers
  unsigned ds;
  unsigned es;
  unsigned fs;
  unsigned gs;		// TODO: does this belong here?
};

struct Iret {		// A frame created when an interrupt occurs
  unsigned error;	// Only a few exceptions generate an error code
  unsigned eip;
  unsigned cs;
  unsigned eflags;
  unsigned esp;		// These two elements appear only if there
  unsigned ss;		// is a change of privilege level
};

struct Context {	// A frame that captures the context of user code
  struct Registers regs;
  struct Segments  segs;
  struct Iret      iret;
};

#define IOPL_SHIFT       12   /* shift for IO protection level in eflags */
#define IF_SHIFT          9   /* shift for interrupt flags in eflags     */
#define FLAGS_RESERVED  0x2   /* Intel reserved bits in eflags           */
#define USER_FLAGS_MASK	(0x01 | 0x04 | 0x10 | 0x40 | 0x80 | 0x800)
                /* CF=0x01, PF=0x04, AF=0x10, ZF=0x40, SF=0x80, OF=0x800 */

/*-------------------------------------------------------------------------
 * Initialize the Context for a user thread.
 */
extern byte USER_CS[], USER_DS[], KERN_CS[], KERN_DS[], UTCB_DS[];

static inline void initContext(
 byte* cs, byte* ds, unsigned eip, unsigned flags, struct Context* ctxt) {
  // Segment registers:
  ctxt->segs.ds     = ctxt->segs.es  = ctxt->segs.fs  =
  ctxt->iret.ss     = (unsigned)ds;
  ctxt->iret.cs     = (unsigned)cs;
  ctxt->segs.gs     = (unsigned)UTCB_DS;
  // Other registers
  ctxt->iret.eflags = flags;
  ctxt->iret.eip    = eip;
  ctxt->iret.esp    = ctxt->iret.error =
  ctxt->regs.edi    = ctxt->regs.esi = ctxt->regs.ebp = ctxt->regs.esp = 
  ctxt->regs.ebx    = ctxt->regs.edx = ctxt->regs.ecx = ctxt->regs.eax = 0;
}

static inline void initUserContext(struct Context* ctxt) {
  unsigned flags = (3<<IOPL_SHIFT | 1<<IF_SHIFT | FLAGS_RESERVED);
  initContext(USER_CS, USER_DS, 0, flags, ctxt);
}

static inline void initIdleContext(struct Context* ctxt, unsigned eip) {
  unsigned flags = (0<<IOPL_SHIFT | 1<<IF_SHIFT | FLAGS_RESERVED);
  initContext(KERN_CS, KERN_DS, eip, flags, ctxt);
}

/*-------------------------------------------------------------------------
 * Return to a user thread.
 */
static inline void returnToContext(struct Context* ctxt) {
  asm("\n movl  %0, %%esp  # Reset stack to base of user context"
      "\n popa             # Restore registers"
      "\n pop   %%ds       # Restore segments"
      "\n pop   %%es"
      "\n pop   %%fs"
      "\n pop   %%gs"
      "\n addl  $4, %%esp  # Skip error code"
      "\n iret             # Return from interrupt\n"
      : : "a"(ctxt));
}

/*-------------------------------------------------------------------------
 * System Call Parameter and Result Bindings.
 */
#define KernelInterface_SetBaseAddress (current->context.regs.eax)
#define KernelInterface_SetAPIVersion  (current->context.regs.ecx)
#define KernelInterface_SetAPIFlags    (current->context.regs.edx)
#define KernelInterface_SetKernelId    (current->context.regs.esi)

#define ExchangeRegisters_GetDest      (current->context.regs.eax)
#define ExchangeRegisters_GetControl   (current->context.regs.ecx)
#define ExchangeRegisters_SP           (current->context.regs.edx)
#define ExchangeRegisters_IP           (current->context.regs.esi)
#define ExchangeRegisters_Flags        (current->context.regs.edi)
#define ExchangeRegisters_UserHandle   (current->context.regs.ebx)
#define ExchangeRegisters_Pager        (current->context.regs.ebp)
#define ExchangeRegisters_SetResult    (current->context.regs.eax)
#define ExchangeRegisters_SetControl   (current->context.regs.ecx)

#define ThreadControl_DestId           (current->context.regs.eax)
#define ThreadControl_SpaceId          (current->context.regs.esi)
#define ThreadControl_SchedulerId      (current->context.regs.edx)
#define ThreadControl_PagerId          (current->context.regs.ecx)
#define ThreadControl_UtcbLocation     (current->context.regs.edi)
#define ThreadControl_Result           (current->context.regs.eax)

#define ThreadSwitch_GetDest           (current->context.regs.eax)

#define Schedule_DestId                (current->context.regs.eax)
#define Schedule_TsLen                 (current->context.regs.ecx)
#define Schedule_TotQuantum            (current->context.regs.edx)
#define Schedule_ProcControl           (current->context.regs.ebx)
#define Schedule_Prio                  (current->context.regs.edi)
#define Schedule_Result                (current->context.regs.eax)
#define Schedule_RemTs                 (current->context.regs.ecx)
#define Schedule_RemQuantum            (current->context.regs.edx)

#define SystemClock_Lo                 (current->context.regs.eax)
#define SystemClock_Hi                 (current->context.regs.edx)

#define IPC_GetTo                      (current->context.regs.eax)
#define IPC_GetFromSpec(tcb)           (tcb    ->context.regs.edx)
#define IPC_SetFrom(tcb)               (tcb    ->context.regs.eax)

#define Unmap_Control                  (current->context.regs.eax)

#define SpaceControl_SpaceSpecifier    (current->context.regs.eax)
#define SpaceControl_Control           (current->context.regs.ecx)
#define SpaceControl_KipArea           (current->context.regs.edx)
#define SpaceControl_UtcbArea          (current->context.regs.esi)
#define SpaceControl_Result            (current->context.regs.eax)

#define ProcessorControl_ProcessorNo   (current->context.regs.eax)
#define ProcessorControl_InternalFreq  (current->context.regs.ecx)
#define ProcessorControl_ExternalFreq  (current->context.regs.edx)
#define ProcessorControl_Voltage       (current->context.regs.esi)
#define ProcessorControl_Result        (current->context.regs.eax)

#define MemoryControl_Control          (current->context.regs.eax)
#define MemoryControl_Attribute0       (current->context.regs.ecx)
#define MemoryControl_Attribute1       (current->context.regs.edx)
#define MemoryControl_Attribute2       (current->context.regs.ebx)
#define MemoryControl_Attribute3       (current->context.regs.ebp)
#define MemoryControl_Result           (current->context.regs.eax)

#endif
/*-----------------------------------------------------------------------*/

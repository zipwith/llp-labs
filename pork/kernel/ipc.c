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
 * IPC and related operations:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "space.h"
#include "threads.h"
#include "hardware.h"

#define DEBUG(cmd)   /* cmd */

/*-------------------------------------------------------------------------
 * Halt a thread, removing it from the runqueue if necessary.  If the
 * target thread is blocked sending, then it should be removed from the
 * corresponding sendqueue before this method is invoked.
 */
void haltThread(struct TCB* tcb) {
  if (tcb->status==Runnable) {
    removeRunnable(tcb);
  }
  tcb->status = Halted;
}

/*-------------------------------------------------------------------------
 * Resume a thread after an IPC operation, either because it has completed
 * successfully, or because it has triggered an error.  The specified
 * thread will either be returned to the appropriate runqueue or else it
 * will be halted if the Halted flag has been set since the IPC began.
 * If the target thread is blocked sending, then it should be removed
 * from the corresponding sendqueue before this method is invoked.
 */
void resumeThread(struct TCB* tcb) {
DEBUG(printf("resumeThread: threadId=%x, status=%x\n", tcb->tid, tcb->status);)
  if (tcb->status & Halted) {        // Was halt requested?
DEBUG(printf("resumeThread: halt was requested\n");)
    tcb->status = Halted;
  } else {                           // Make it runnable again ...
DEBUG(extern void showRunqueue();)
    if (tcb->status!=Runnable) {
DEBUG(printf("resumeThread: inserting into run queue\n");)
      insertRunnable(tcb);
      tcb->status = Runnable;
    }
DEBUG(printf("resumeThread: thread %x is now runnable\n", tcb->tid);)
DEBUG(showRunqueue();)
  }
}

/*-------------------------------------------------------------------------
 * Terminate an IPC operation for a given thread as a result of an error
 * during its send phase.
 */
void sendError(IPCType sendtype, struct TCB* send, IPCErr err) {
  if (sendtype==MRs) {
    send->utcb->errorCode = IPCErrCode(err, DuringSend);
    send->utcb->mr[0]     = IPCErrBit;
    resumeThread(send);
  } else {
    haltThread(send);
  }
}

/*-------------------------------------------------------------------------
 * Terminate an IPC operation for a given thread as a result of an error
 * during its receive phase.
 */
void recvError(IPCType recvtype, struct TCB* recv, IPCErr err) {
DEBUG(printf("recvError, type=%d\n", recvtype);)
  if (recvtype==MRs || recvtype==Startup) { // TODO: is Startup ok with L4 spec?
    recv->utcb->errorCode = IPCErrCode(err, DuringReceive);
    recv->utcb->mr[0]     = IPCErrBit; // TODO: should this do |= IPCErrBit?
    resumeThread(recv);
  } else {
    haltThread(recv);
  }
}

/*-------------------------------------------------------------------------
 * IPC Support: TODO: special treatment of message regs MR0, MR1, MR2, ...
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Transfer a typed item from one thread to another.
 */
static IPCErr transferTyped(
 struct TCB* send, struct TCB* recv, Fpage acc, unsigned t0, unsigned t1) {
DEBUG(printf("TRANSFER Typed Item [%x->%x] with t0=%x, t1=%x\n", send->tid, recv->tid, t0, t1);)
  if ((t0&0x3fe)==0x8) {        // MapItem?
    if (availPages(2)) {
DEBUG(extern void showSpace(struct Space* space);)
      map2(send->space, (Fpage)t1, align(t0,10), recv->space, acc);
DEBUG(printf("Completed transfer of MapItem from %x to %x:\n", send->tid, recv->tid);)
DEBUG(showSpace(recv->space);)
DEBUG(printf("----------------------------\n");)
      return NoError;
    }
    return MessageOverflow;
  } else if ((t0&0x3fe)==0xa) { // GrantItem?
    // TODO: fill this in
  }
DEBUG(printf("IGNORING Typed Item with t0=%x, t1=%x\n", t0, t1);)
  return Protocol;
}

/*-------------------------------------------------------------------------
 * Transfer a message between two threads.
 */
static IPCErr transferMessage(
 IPCType sendtype, struct TCB* send, IPCType recvtype, struct TCB* recv) {
DEBUG(printf("transferMessage: sendtype=%d, recvtype=%d\n", sendtype, recvtype);)
  if (recvtype==MRs) {             // Send to MRs (Destination is user ipc)
    struct UTCB* rutcb = recv->utcb;
    IPC_SetFrom(recv)  = send->tid;            // save "from" thread id
    switch (sendtype) {
      case MRs : {                // Send between sets of message registers
          struct UTCB* sutcb = send->utcb;
          unsigned u         = mask(sutcb->mr[0],    6);   // untyped items
          unsigned t         = mask(sutcb->mr[0]>>6, 6);   // typed items
          if ((u+t>=NUMMRS) || (t&1)) {
            // TODO: Set mr[0] ?
            return MessageOverflow;
          } else {
            unsigned i;
            rutcb->mr[0] = MsgTag(sutcb->mr[0]>>16, 0, t, u);
            for (i=1; i<=u; i++) {
              rutcb->mr[i] = sutcb->mr[i];
            }
            if (t>0) {
              Fpage acc = rutcb->acceptor;
              do {
                IPCErr err = transferTyped(send, recv, acc,
                               rutcb->mr[i]   = sutcb->mr[i],
                               rutcb->mr[i+1] = sutcb->mr[i+1]);
                if (err!=NoError) {
                  // TODO: rewrite MR0 to reflect actual u, t value?
                  return err;
                }
                i += 2;
              } while ((t-=2)>0);
            }
            return NoError;
          }
        }

      case PageFault : { // Send pagefault message to pager
          unsigned rwx  = (send->context.iret.error & 2) ? 2 : 4;
          rutcb->mr[0]  = MsgTag(((-2)<<4)|rwx, 0, 0, 2);
          rutcb->mr[1]  = send->faultCode;
          rutcb->mr[2]  = send->context.iret.eip;
        }
        return NoError;

      case Exception :   // Send message to an exception handler
        rutcb->mr[0]  = MsgTag(((-4)<<4), 0, 0, 12);
        rutcb->mr[1]  = send->context.iret.eip;
        rutcb->mr[2]  = send->context.iret.eflags;
        rutcb->mr[3]  = send->faultCode;
        rutcb->mr[4]  = send->context.iret.error;
        rutcb->mr[5]  = send->context.regs.edi;
        rutcb->mr[6]  = send->context.regs.esi;
        rutcb->mr[7]  = send->context.regs.ebp;
        rutcb->mr[8]  = send->context.regs.esp;
        rutcb->mr[9]  = send->context.regs.ebx;
        rutcb->mr[10] = send->context.regs.edx;
        rutcb->mr[11] = send->context.regs.ecx;
        rutcb->mr[12] = send->context.regs.eax;
        return NoError;

      case Interrupt :   // Send message to an interrupt handler
        rutcb->mr[0] = MsgTag((-1)<<4, 0, 0, 0);
        return NoError;

      case Preempt   :   // Send preemption message to thread scheduler
        // NOT YET IMPLEMENTED
        break;

      case Startup   :   // Startup is only used for receiving.
	break;
    }

  } else if (sendtype==MRs) {   // Receive from MRs (Source is user ipc)
    struct UTCB* sutcb = send->utcb;
    switch (recvtype) {
      case PageFault :  // Receive a response from a pager
        if (mask(sutcb->mr[0],12)==MsgTag(0, 0, 2, 0)) {
          return transferTyped(send, recv,
                       completeFpage(), sutcb->mr[1], sutcb->mr[2]);
        }
        break;

      case Exception :   // Receive a response from an exception handler
        if (mask(sutcb->mr[0], 12)==MsgTag(0, 0, 0, 12)) {
          recv->context.iret.eip    = sutcb->mr[1];
          recv->context.iret.eflags = sutcb->mr[2] & USER_FLAGS_MASK;
          // ignore mr[3] (exceptionNo) and mr[4] (error code) on return
          recv->context.regs.edi    = sutcb->mr[5];
          recv->context.regs.esi    = sutcb->mr[6];
          recv->context.regs.ebp    = sutcb->mr[7];
          recv->context.regs.esp    = sutcb->mr[8];
          recv->context.regs.ebx    = sutcb->mr[9];
          recv->context.regs.edx    = sutcb->mr[10];
          recv->context.regs.ecx    = sutcb->mr[11];
          recv->context.regs.eax    = sutcb->mr[12];
          return NoError;
        }
        break;

      case Interrupt :   // Receive a response from an interrupt handler
        if (mask(sutcb->mr[0],12)==0) {
          ASSERT(mask(recv->tid, VERSIONBITS)==1, "Wrong irq version");
          ASSERT(threadNo(recv->tid) < NUMIRQs,   "IRQ out of range");
          enableIRQ(threadNo(recv->tid));   // Reenable interrupt
          return NoError;
        }
        break;

      case Startup   :   // Receive startup message from thread's pager
        if (mask(sutcb->mr[0],12)==MsgTag(0, 0, 0, 2)) {
          recv->context.iret.eip = sutcb->mr[1];
          recv->context.iret.esp = sutcb->mr[2];
          return NoError;
        }
        break;

      case Preempt   : // User threads cannot send preemption message
      case MRs       : // case for recvtype==MRs was dealt with above!
        break;
    }
  }
  return Protocol;     // Protocol error: incompatible types/format
}

/*-------------------------------------------------------------------------
 * Determine whether a particular IPC send operation is allowed to block.
 */
static inline bool sendCanBlock(IPCType sendtype, struct TCB* send) {
  return (sendtype!=MRs) || (send->context.regs.esi & IPCSendBlock);
}

/*-------------------------------------------------------------------------
 * Return the from specifier for an IPC receive.
 */
static inline ThreadId recvFromSpec(IPCType recvtype, struct TCB* recv) {
  switch (recvtype) {
    case MRs       : return IPC_GetFromSpec(recv);
    case Exception : return recv->utcb->exceptionHandler;
    case Interrupt : return (ThreadId)(recv->vutcb);
    case PageFault :
    case Startup   : return recv->utcb->pager;
    default        : return nilthread;
  }
}

/*-------------------------------------------------------------------------
 * Determine whether a particular IPC receive operation is allowed to block.
 */
static inline bool recvCanBlock(IPCType recvtype, struct TCB* recv) {
  return (recvtype!=MRs) || (recv->context.regs.esi & IPCRecvBlock);
}

/*-------------------------------------------------------------------------
 * Implements the send phase of IPC, given a pointer to the sender's tcb,
 * the IPC type, and an id for the destination.  We assume that the sender
 * is either Runnable or Halted (the Interrupt case) when this method is
 * called.  Return result is true if a message was transferred.
 */
static bool sendPhase(IPCType sendtype, struct TCB* send, ThreadId recvId) {
  // Find the receiver TCB: -----------------------------------------------
  struct TCB* recv;
DEBUG(printf("Send %x: type %d to %x\n", send->tid, sendtype, recvId);)
  if (recvId==anythread      ||
      recvId==anylocalthread ||
      !(recv=findTCB(recvId))) {
DEBUG(printf("Send %x: NonExistingPartner\n", send->tid);)
    sendError(sendtype, send, NonExistingPartner);
    return 0;
  }

DEBUG(printf("Send %x: Partner status %x\n", send->tid, recv->status);)
  // Determine whether we can send the message immediately: ---------------
  if (isReceiving(recv)) {
    IPCType  recvtype = ipctype(recv);
    ThreadId srcId    = recvFromSpec(recvtype, recv);
DEBUG(printf("Send %x: Partner is Receiving (type %d) ... \n", send->tid, recvtype);)
DEBUG(printf("Send %x: Partner is %x ... \n", send->tid, srcId);)
    if ((srcId==send->tid) ||
        (srcId==anythread) ||
        (srcId==anylocalthread && send->space==recv->space)) {
      // Destination is blocked and ready to receive from send:
DEBUG(printf("Send %x: Transferring message ... \n", send->tid);)
      IPCErr err = transferMessage(sendtype, send, recvtype, recv);
      if (err==NoError) {
DEBUG(printf("Send %x: Successful, resuming thread\n", send->tid);)
        resumeThread(recv);
        return 1;
      } else {
DEBUG(printf("Send %x: Error occurred %d\n", send->tid, err);)
        sendError(sendtype, send, err);
        recvError(recvtype, recv, err);
        return 0;
      }
    }
DEBUG(printf("Send %x: ... But not from sender\n", send->tid);)
  }

  // Destination is not ready to receive a message, so try to block: ------
  if (sendCanBlock(sendtype, send)) {
DEBUG(printf("Send %x: Blocking\n", send->tid);)
    if (send->status==Runnable) {
      removeRunnable(send);
    }
    send->status    = Sending(sendtype) | (Halted & send->status);
    send->receiver  = recv;
    recv->sendqueue = insertTCB(recv->sendqueue, send);
  } else {
DEBUG(printf("Send %x: Blocking not allowed, declaring NoPartner\n", send->tid);)
    sendError(sendtype, send, NoPartner);
  }
  return 0;
}

/*-------------------------------------------------------------------------
 * Implements the receive phase of IPC, given a pointer to the receiver's
 * tcb, the IPC type, and an id for the source.
 */
static void recvPhase(IPCType recvtype, struct TCB* recv, ThreadId fromSpec) {
  for (;;) {
DEBUG(printf("Recv %x: type %d from %x\n", recv->tid, recvtype, fromSpec);)
    // Search for a partner: ----------------------------------------------
    struct TCB* send;
    if (fromSpec==anythread) {
      if ((send=recv->sendqueue)) {
        goto rendezvous;
      }
    } else if (fromSpec==anylocalthread) {
      if ((send=recv->sendqueue)) {
        struct TCB*   first = send;
        struct Space* local = recv->space;
        do {
          if (send->space == local) {
            goto rendezvous;
          }
        } while ((send=send->next)!=first);
      }
    } else if (!(send=findTCB(fromSpec))) {
      recvError(recvtype, recv, NonExistingPartner);
      return;
    } else if (isSending(send) && send->receiver==recv) {
      goto rendezvous;
    }

DEBUG(printf("Recv %x: partner is not ready\n", recv->tid);)
    // Block: -------------------------------------------------------------
    if (recvCanBlock(recvtype, recv)) {
DEBUG(printf("Recv %x: Blocking ...\n", recv->tid);)
      if (recv->status==Runnable) {
        removeRunnable(recv);
        recv->status = Receiving(recvtype) | (Halted & recv->status);
      }
    } else {
DEBUG(printf("Recv %x: Blocking not allowed, declaring NoPartner\n", recv->tid);)
      recvError(recvtype, recv, NoPartner);
    }
    return;

    // Rendezvous: --------------------------------------------------------
rendezvous:;
DEBUG(printf("Recv %x: Transferring message ...\n", recv->tid);)
    IPCType sendtype = ipctype(send);
    IPCErr  err      = transferMessage(sendtype, send, recvtype, recv);
    recv->sendqueue  = removeTCB(recv->sendqueue, send);
    if (err!=NoError) {      // Error during message transfer?
DEBUG(printf("Recv %x: Transfer fails, ending IPC ...\n", recv->tid);)
      sendError(sendtype, send, err);
      recvError(recvtype, recv, err);
      return;
    }
    resumeThread(recv);

    // Finished with receiver, but maybe the sender we've just paired
    // with during rendez-vous is now ready to receive itself ...
    if ((fromSpec=recvFromSpec(sendtype, send))==nilthread) {
DEBUG(printf("Recv %x: sender has no receive phase so we're done ...\n", recv->tid);)
      resumeThread(send);  // send has no receive phase
      return;
    }
    recv         = send;
    recvtype     = sendtype;
    recv->status = Receiving(recvtype) | (Halted & recv->status);
  }
}

/*-------------------------------------------------------------------------
 * The "IPC" System Call:
 *-----------------------------------------------------------------------*/
ENTRY ipc() {
DEBUG(printf("kernel: ipc(%x) to: %x from: %x - %x [%x, %x, ...]\n", current->tid, IPC_GetTo, IPC_GetFromSpec(current), current->utcb->mr[0], current->utcb->mr[1], current->utcb->mr[2]);)
  ThreadId to = IPC_GetTo;                       // Send Phase
DEBUG(printf("ipc system call, sendphase to=%x\n", to);)
  if (to!=nilthread) {
DEBUG(printf("non-null sendphase\n");)
    if (!sendPhase(MRs, current, to)) {
      reschedule();
    }
  }
  ThreadId fromSpec = IPC_GetFromSpec(current);  // Receive Phase
DEBUG(printf("ipc system call, recvphase  from=%x\n", fromSpec);)
  if (fromSpec!=nilthread) {
DEBUG(printf("non-null recvphase\n");)
    current->utcb->mr[0] = 0;
    recvPhase(MRs, current, fromSpec);
  }
DEBUG(printf("ipc system call done\n");)
  reschedule();
}

/*-------------------------------------------------------------------------
 * Handlers for system exceptions and interrupts:
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Generate an IPC in response to an exception triggered by the current
 * thread.  (Of course, current is Runnable when we begin, or else it
 * wouldn't have been able to produce an exception.)
 */
static void handleException(int exn) {
  current->faultCode = exn;    // Save the exception number
DEBUG(printf("EXCEPTION: %d\n", exn);)
  ThreadId destId    = current->utcb->exceptionHandler;
  if (destId==nilthread) {
    haltThread(current);
  } else if (sendPhase(Exception, current, destId)) {
    removeRunnable(current);   // Block current if message already delivered
    current->status = Receiving(Exception);
  }
  refreshSpace();
  reschedule();
}

ENTRY divideError()                { handleException(0);  }
ENTRY debug()                      { handleException(1);  }
ENTRY nmiInterrupt()               { handleException(2);  }
ENTRY breakpoint()                 { handleException(3);  }
ENTRY overflow()                   { handleException(4);  }
ENTRY boundRangeExceeded()         { handleException(5);  }
ENTRY deviceNotAvailable()         { handleException(7);  }
ENTRY doubleFault()                { handleException(8);  }
ENTRY coprocessorSegmentOverrun()  { handleException(9);  }
ENTRY invalidTSS()                 { handleException(10); }
ENTRY segmentNotPresent()          { handleException(11); }
ENTRY stackSegmentFault()          { handleException(12); }
ENTRY generalProtection()          { handleException(13); }
ENTRY floatingPointError()         { handleException(16); }
ENTRY alignmentCheck()             { handleException(17); }
ENTRY machineCheck()               { handleException(18); }
ENTRY simdFloatingPointException() { handleException(19); }

/*-------------------------------------------------------------------------
 * Generate an IPC in response to a page fault in the current thread.
 */
ENTRY pageFault() {
  asm("  movl %%cr2, %0\n" : "=r"(current->faultCode));
// abortIf(((current->faultCode)&4)==0, "page fault in kernel mode");
   printf("page fault handler: eip=%x, error=%x, fault addr=%x\n",
    current->context.iret.eip, current->context.iret.error, current->faultCode);
  if (current->space==sigma0Space && sigma0map(current->faultCode)) {
    printf("sigma0 case succeeded!\n");
  } else {
    ThreadId  pagerId = current->utcb->pager;
DEBUG(printf("pagerId=%x\n", pagerId);)
    if (pagerId==nilthread) {
      haltThread(current);
    } else if (sendPhase(PageFault, current, pagerId)) {
DEBUG(extern void showRunqueue();)
DEBUG(printf("SendPhase to %x succeeded, removing current=%x from runqueue\n", pagerId,current->tid);)
      removeRunnable(current);   // Block current if message already delivered
DEBUG(showRunqueue();)
      current->status = Receiving(PageFault);
    }
DEBUG(else { printf("SendPhase to %x did not succeed\n", pagerId); })
  }
  refreshSpace();
  reschedule();
}

/*-------------------------------------------------------------------------
 * Handle an invalid opcode exception.  This is a special case that tests
 * for the LOCK NOP instruction (get kernel interface page) before handing
 * over to a generic exception handler.
 */
ENTRY invalidOpcode() {
  byte* eip = (byte*)current->context.iret.eip;
  // TODO: Confirm that (1) this works; and (2) it can't page fault!
  if (eip[0]==0xf0 && eip[1]==0x90) { // Check for LOCK NOP instruction
    current->context.iret.eip += 2;   // found => KernelInterface syscall
    KernelInterface_SetBaseAddress = kipStart(current->space);
    KernelInterface_SetAPIVersion  = API_VERSION;
    KernelInterface_SetAPIFlags    = API_FLAGS;
    KernelInterface_SetKernelId    = KERNEL_ID;
    resume();
  }
  handleException(6);
}

/*-------------------------------------------------------------------------
 * Handle a hardware interrupt.
 */
ENTRY hardwareIRQ() {
  unsigned n = current->context.iret.error;
DEBUG(printf("Hardware IRQ %d\n", n);)
  maskAckIRQ(n); // Mask and acknowledge the interrupt with the PIC
  struct TCB* irqTCB = existsTCB(n);
  // An irq thread may be Halted, Sending (i.e., waiting for the user level
  // handler to receive notice of the interrupt), or Receiving (i.e., waiting
  // for the user level handler to finish processing the interrupt and send
  // an acknowledgement).  In theory, an interrupt can only occur if it is
  // unmasked at the PIC, and that, in turn, should only be possible when
  // (1) the corresponding irq thread is Halted; and (2) the "pager" for
  // the irq thread (stored in the vutcb field) is set to a non-nilthread id.
  // TODO: Can irq threads be accessed through exchangeregisters?
  // Can irq threads be used in spacecontrol (to create other threads
  // in irq space, for example)?  ...
  if (irqTCB->status==Halted && irqTCB->vutcb!=nilthread) {
    if (sendPhase(Interrupt, irqTCB, irqTCB->vutcb)) {
      irqTCB->status = Receiving(Interrupt) | Halted;
    }
  }
  reschedule(); // allow the user level handler to begin ...
}

/*-------------------------------------------------------------------------
 * The "ExchangeRegisters" System Call:
 *-----------------------------------------------------------------------*/
#define ExchangeRegisters_H                (1<<0)
#define ExchangeRegisters_R                (1<<1)
#define ExchangeRegisters_S                (1<<2)
#define ExchangeRegisters_s                (1<<3)
#define ExchangeRegisters_i                (1<<4)
#define ExchangeRegisters_f                (1<<5)
#define ExchangeRegisters_u                (1<<6)
#define ExchangeRegisters_p                (1<<7)
#define ExchangeRegisters_h                (1<<8)
#define ExchangeRegisters_d                (1<<9)
#define ExchangeRegisters_r                (1<<10)

ENTRY exchangeRegisters() {
  struct TCB* dest = findTCB(ExchangeRegisters_GetDest);
DEBUG(printf("ExchangeRegisters: %x\n", dest->tid);)
  if (dest && dest->utcb &&                 // destination exists and active
      (dest->space==current->space ||       // is in current space
       privileged(current->space)  ||       // current is privileged
       dest->utcb->pager==current->tid)) {  // or destination's pager
    // Valid thread id for thread in current address space
    unsigned incontrol  = ExchangeRegisters_GetControl;
    unsigned outcontrol = 0;
    unsigned oldsp      = dest->context.iret.esp; // capture original values
    unsigned oldip      = dest->context.iret.eip;
    unsigned oldflags   = dest->context.iret.eflags & USER_FLAGS_MASK;
    unsigned oldhandle  = dest->utcb->userDefinedHandle;
    unsigned oldpager   = dest->utcb->pager;

DEBUG(printf("ExchangeRegisters control = %x\n", incontrol);)
    // Update user level registers: ---------------------------------------
    if (incontrol & ExchangeRegisters_s) {        // Stack pointer
      dest->context.iret.esp = ExchangeRegisters_SP;
DEBUG(printf("ExchangeRegisters set sp = %x\n", dest->context.iret.esp);)
    }
    if (incontrol & ExchangeRegisters_i) {        // Instruction Pointer
      dest->context.iret.eip = ExchangeRegisters_IP;
DEBUG(printf("ExchangeRegisters set ip = %x\n", dest->context.iret.eip);)
    }
    if (incontrol & ExchangeRegisters_f) {        // User Flags
      dest->context.iret.eflags
                       = (dest->context.iret.eflags  & (~USER_FLAGS_MASK)) 
                       | (ExchangeRegisters_Flags & USER_FLAGS_MASK);
DEBUG(printf("ExchangeRegisters set flags = %x\n", dest->context.iret.eflags);)
    }
    if (incontrol & ExchangeRegisters_u) {        // User defined handle
      dest->utcb->userDefinedHandle = ExchangeRegisters_UserHandle;
    }
    if (incontrol & ExchangeRegisters_p) {        // Pager
      dest->utcb->pager = ExchangeRegisters_Pager;
    }

    // Change thread status: ----------------------------------------------
    if (dest->status==Halted) {                   // dest halted?
      outcontrol |= ExchangeRegisters_H;
    }
    if (incontrol & ExchangeRegisters_h) {        // Halt/Resume thread
      if (incontrol & ExchangeRegisters_H) {
        if (!(dest->status & Halted)) {           // Halt thread
          if (dest->status==Runnable) {
            removeRunnable(dest);
          }
          dest->status |= Halted;                 // Do NOT halt ongoing IPC
        }
      } else if (dest->status==Halted) {          // Resume halted thread
        insertRunnable(dest);
        dest->status = Runnable;
      }
    }

    if (isReceiving(dest)) {                      // dest in IPC receive?
DEBUG(printf("ExchangeRegisters is receiving\n");)
      outcontrol |= ExchangeRegisters_R;
      if (incontrol & ExchangeRegisters_R) {      // cancel IPC receive
DEBUG(printf("ExchangeRegisters cancelling receive\n");)
        recvError(ipctype(dest), dest, Cancelled);
      }
    } else if (isSending(dest)) {                 // dest in IPC send?
DEBUG(printf("ExchangeRegisters is sending\n");)
      outcontrol |= ExchangeRegisters_S;
      if (incontrol & ExchangeRegisters_S) {      // cancel IPC send
        struct TCB* recv = dest->receiver; // remove from send queue
DEBUG(printf("ExchangeRegisters cancelling send\n");)
        recv->sendqueue  = removeTCB(recv->sendqueue, dest);
        sendError(ipctype(dest), dest, Cancelled);
      }
    }

    // Return original values: --------------------------------------------
    if (incontrol & ExchangeRegisters_d) {
      ExchangeRegisters_SP         = oldsp;
      ExchangeRegisters_IP         = oldip;
      ExchangeRegisters_Flags      = oldflags;
      ExchangeRegisters_UserHandle = oldhandle;
      ExchangeRegisters_Pager      = oldpager;
      ExchangeRegisters_SetControl = outcontrol;
    }
    ExchangeRegisters_SetResult = dest->tid;

    // If dest==current, then it is possible that we could have caused the
    // current thread to become Halted ... in which case we need to go to
    // the scheduler instead of returning to the user thread.
    if (current->status!=Runnable) {
      reschedule();
    }
DEBUG(printf("ExchangeRegisters outcontrol = %x\n", outcontrol);)
  } else {
    current->utcb->errorCode    = INVALID_THREADID;
    ExchangeRegisters_SetResult = nilthread;
DEBUG(printf("ExchangeRegisters invalid threadid\n");)
  }
  resume();
}

/*-----------------------------------------------------------------------*/

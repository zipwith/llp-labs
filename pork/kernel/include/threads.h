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
 * Thread Data Structures:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef THREADS_H
#define THREADS_H
#include "kip.h"
#include "space.h"
#include "context.h"

/*-------------------------------------------------------------------------
 * Thread Ids:
 *-----------------------------------------------------------------------*/
typedef unsigned       ThreadId;                    // Global thread id
#define nilthread      0
#define anythread      (-1)
#define anylocalthread ((-1)<<6)
#define threadId(t,v)  ((t<<VERSIONBITS)|v)
#define threadNo(tid)  mask((tid)>>VERSIONBITS, THREADBITS)
#define isGlobal(tid)  (mask(tid,6))

/*-------------------------------------------------------------------------
 * System Call Error Codes:
 *-----------------------------------------------------------------------*/
typedef enum {
 NO_PRIVILEGE=1, INVALID_THREADID=2, INVALID_SPACE=3, INVALID_SCHEDULER=4,
 INVALID_PARAMETER=5, INVALID_UTCB=6, INVALID_KIPAREA=7, OUT_OF_MEMORY=8,
 INVALID_REDIRECTOR=9
} SyscallErr;

/*-------------------------------------------------------------------------
 * User-level thread control blocks (UTCBs):
 *-----------------------------------------------------------------------*/
struct UTCB {
  unsigned reserved0[48];
  ThreadId myGlobalId;
  unsigned notifyMask;
  unsigned notifyBits;
  Fpage    acceptor;
  unsigned processorNo;
  unsigned userDefinedHandle;
  ThreadId pager;
  ThreadId exceptionHandler;
  unsigned preemptCopFlags;
  unsigned errorCode;
  unsigned intendedReceiver;
  unsigned virtualSender;
  unsigned preemptCallbackIP;
  unsigned preemptedIP;
  unsigned reserved1[2];
  unsigned mr[NUMMRS];
};

/*-------------------------------------------------------------------------
 * Kernel thread control blocks (TCBs):
 *-----------------------------------------------------------------------*/
struct TCB {
  ThreadId       tid;           // this thread's id and version number
  byte           status;        // thread status
  byte           prio;          // thread priority
  byte           padding;
  byte           count;	        // for gc of TCBs in kernel memory
  struct UTCB*   utcb;          // pointer to this thread's utcb
  unsigned       vutcb;         // virtual address of utcb

  struct TCB*    sendqueue;     // list of threads waiting to send
  struct TCB*    receiver;      // pointer to owner of sendqueue
  struct TCB*    prev;
  struct TCB*    next;

  struct Space*  space;         // pointer to this thread's addr space
  unsigned       faultCode;     // exception number or page fault addr
  struct Context context;       // context of user level process

  ThreadId       scheduler;     // scheduling parameters
  unsigned       timeslice;
  unsigned       timeleft;
  unsigned       quantleft;
};

extern struct TCB* current;	// Points to the TCB of the current thread

extern void        initTCBs(void);
extern void        initScheduling(void);
extern struct TCB* allocTCB1(ThreadId tid, struct Space* space, ThreadId scheduler);
extern struct TCB* existsTCB(unsigned threadNo);
extern struct TCB* findTCB(ThreadId tid);
extern struct TCB* insertTCB(struct TCB* queue, struct TCB* tcb);
extern struct TCB* removeTCB(struct TCB* queue, struct TCB* tcb);
extern void        insertRunnable(struct TCB* tcb);
extern void        removeRunnable(struct TCB* tcb);
extern void        haltThread(struct TCB* tcb);
extern void        resumeThread(struct TCB* tcb);
extern void        reschedule(void);
static inline void resume(void) { returnToContext(&(current->context)); }

#define retError(result, code)  do { result = 0; \
                                     current->utcb->errorCode = code; \
                                     resume(); } while (0)

/*-------------------------------------------------------------------------
 * Thread status:
 * A thread can be in one of five possible states:
 * - Inactive: there is no UTCB for the thread, so it cannot run.
 * - Halted: the thread has a UTCB but its status is set to "Halted"
 *   and it is not listed in any of the runqueues or in any thread's
 *   sendqueue.  (We could set up a queue of halted threads to help
 *   with debugging/dumping system state.)
 * - Runnable: the thread is ready for execution and is currently
 *   stored in one of the (priority-indexed) runqueues with status
 *   set to "Runnable".
 * - Receiving: the thread is blocked waiting to receive a message.
 *   The thread status is set to "Receiving(type)" where type is one
 *   of MRs, Startup, Exception, PageFault, or Interrupt, and is not
 *   listed in any runqueue or sendqueue.  (We could set up a queue
 *   of receiving threads to help with debugging/dumping system state.)
 * - Sending: the thread is blocked waiting to send a message.  The
 *   thread status is set to "Sending(type)", where type is one of
 *   MRs, Preempt, Exception, PageFault, or Interrupt, and it is
 *   linked in to the sendqueue of its target thread.
 * 
 * A byte field in each TCB specifies the current status of that thread:
 * +----+----+----+---------+
 * | b6 | b5 | b4 | ipctype |
 * +----+----+----+---------+
 * b3-b0: ipctype (4 bits)
 * b4: 1=>halted, or halt requested (i.e., will halt after IPC)
 * b5: 1=>blocked waiting to send an ipc of the specified type
 * b6: 1=>blocked waiting to receive an ipc of the specified type
 * A zero status byte indicates that the thread is Runnable.
 *-----------------------------------------------------------------------*/
#define Runnable         0
#define Halted           0x10
#define Sending(type)    (0x20|(type))
#define Receiving(type)  (0x40|(type))

static inline bool isSending(struct TCB* tcb) {
  return (tcb->status) & 0x20;
}

static inline bool isReceiving(struct TCB* tcb) {
  return (tcb->status) & 0x40;
}

/*-------------------------------------------------------------------------
 * IPC Support:
 *-----------------------------------------------------------------------*/
#define IPCPropBit   (1<<12)   // MR0 input flags: Propagated IPC
#define IPCNotifyBit (1<<13)   //                  Notification requested
#define IPCRecvBlock (1<<14)   //                  Receive can block
#define IPCSendBlock (1<<15)   //                  Send can block
#define IPCErrBit    (1<<15)   // MR output flags: Error occurred

#define MsgTag(label, flags, t, u) (((label)<<16)|((flags)<<12)|((t)<<6)|(u))

typedef enum {
  MRs, PageFault, Exception, Interrupt, Preempt, Startup
} IPCType;

static inline IPCType ipctype(struct TCB* tcb) {
  return (IPCType)(tcb->status & 0xf);
}

typedef enum {
  NoError =0, NoPartner = 1, NonExistingPartner = 2, Cancelled = 3,
  MessageOverflow = 4, NotAccepted = 5,  Protocol = 6, Aborted = 7
} IPCErr;

typedef enum {
  DuringSend = 0, DuringReceive = 1
} IPCErrPhase;

static inline unsigned IPCErrCode(IPCErr err, IPCErrPhase phase) {
  return ((err<<1) | phase);
}

extern void sendError(IPCType sendtype, struct TCB* send, IPCErr err);
extern void recvError(IPCType recvtype, struct TCB* recv, IPCErr err);

#endif
/*-----------------------------------------------------------------------*/

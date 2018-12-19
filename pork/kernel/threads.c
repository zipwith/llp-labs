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
 * Thread Data Structures and Operations:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "space.h"
#include "threads.h"
#include "hardware.h"

#define DEBUG(cmd)   /*cmd*/

/*-------------------------------------------------------------------------
 * Thread Directory and Interrupt Thread Data Structures:
 *-----------------------------------------------------------------------*/
#define TCBDIRBITS 5
#define TCBPAGES   (1<<(THREADBITS - TCBDIRBITS))
typedef struct TCB TCBTable[1<<TCBDIRBITS];
static TCBTable* tcbDir[TCBPAGES];

/*-------------------------------------------------------------------------
 * Create an executable kernel thread (sigma0 or the root task):
 */
static struct TCB*  kernelThread5(
 unsigned tno, struct Space* space, unsigned ip) {
  configureSpace(space, fpage(align(PRIV_KIPADDR, PAGESIZE), PAGESIZE),
                        fpage(PRIV_UTCBADDR, PAGESIZE));
  ThreadId tid    = threadId(tno, 1);   // Create new thread:
  struct TCB* tcb = allocTCB1(tid, space, tid);
  ASSERT(validUtcb(space, PRIV_UTCBADDR), "Invalid kernel thread space");
  tcb->vutcb      = PRIV_UTCBADDR;      // Activate and initialize UTCB
  tcb->utcb       = allocUtcb4(space, PRIV_UTCBADDR);
  tcb->utcb->myGlobalId = tid;
  tcb->context.iret.eip = ip;
  insertRunnable(tcb);
  tcb->status     = Runnable;
DEBUG(printf("Runnable tcb: tid=%x, utcb=%x, ip=%x\n", tid, PRIV_UTCBADDR, ip);)
  return tcb;
}

/*-------------------------------------------------------------------------
 * Initialization of main thread structures:
 */
void initTCBs() {
  // Basic consistency checks:
  ASSERT(sizeof(struct TCB)  <= (1<<(PAGESIZE-TCBDIRBITS)), "TCB size error");
  ASSERT(sizeof(TCBTable)    == (1<<PAGESIZE), "TCBTable size error");
  ASSERT(sizeof(struct UTCB) == (1<<UTCBSIZE), "UTCB size error");

  // Initialization of TCB directory: -------------------------------------
  for (unsigned i=0; i<TCBPAGES; ++i) {
    tcbDir[i] = 0;
  }
  initScheduling();

  // Construct Sigma0 thread: ---------------------------------------------
  abortIf(!availPages(5), "Failed to allocate sigma0 thread");
DEBUG(printf("Making Sigma0 tcb:\n");)
  struct TCB* sigma0tcb
    = kernelThread5(USERBASE, sigma0Space, Sigma0Server.ip);

  // Construct roottask thread: -------------------------------------------
  if (RootServer.ip) {
    abortIf(!availPages(5), "Failed to allocate root thread");
DEBUG(printf("Making Roottask tcb:\n");)
    struct TCB* roottcb
      = kernelThread5(USERBASE+1, rootSpace, RootServer.ip);
    roottcb->utcb->pager = sigma0tcb->tid;  // set pager to sigma0 
DEBUG(printf("roottask, ip=%x, pager=%x\n",RootServer.ip,sigma0tcb->tid);)
  }

  // Construct IRQ threads: -----------------------------------------------
  abortIf(!availPages(1+NUMIRQs), "Failed to allocate interrupt threads");
  struct Space* irqSpace = allocSpace1();
  for (unsigned i=0; i<NUMIRQs; ++i) {
    ThreadId tid    = threadId(i,1);
    struct TCB* tcb = allocTCB1(tid, irqSpace, tid);
    tcb->vutcb      = tid; // no handler for this interrupt
  }
}

/*-------------------------------------------------------------------------
 * Find a pointer to the tcb for the thread with a given thread number.
 * A null pointer is returned if either (a) there is no memory allocated
 * for the specified tcb; or (b) memory has been allocated but the space
 * field is null.  The latter occurs if the tcb memory was allocated
 * previously to store tcbs for other thread numbers in the same range.
 */
struct TCB* existsTCB(unsigned threadNo) {
  TCBTable* tab = tcbDir[threadNo>>TCBDIRBITS];
  if (tab) {
    struct TCB* tcb = ((struct TCB*)tab) + mask(threadNo, TCBDIRBITS);
    if (tcb->space) {
      return tcb;
    }
  }
  return 0;
}

/*-------------------------------------------------------------------------
 * Find a pointer to the TCB for a thread with a given id.  A null pointer
 * is returned if either (a) there is no valid tcb for a thread with the
 * given thread id; or (b) there is a valid tcb but its version number does
 * not match the version in the specified thread id.
 */
struct TCB* findTCB(ThreadId tid) {
  struct TCB* tcb = existsTCB(threadNo(tid));
  return (tcb && tcb->tid==tid) ? tcb : 0;
}

/*-------------------------------------------------------------------------
 * Allocate memory for a TCB with the given thread number in the specified
 * address space.  We assume that the space is not null and that there is
 * no existing TCB for a thread with the same thread number.
 */
struct TCB* allocTCB1(ThreadId tid, struct Space* space, ThreadId scheduler) {
  unsigned    threadNo = threadNo(tid);
  TCBTable*   tab      = tcbDir[threadNo>>TCBDIRBITS];
  if (!tab) {
    tab = tcbDir[threadNo>>TCBDIRBITS] = (TCBTable*)allocPage1();
  }
  ++tab[0]->count;  // Count an additional TCB in this page
  struct TCB* tcb = ((struct TCB*)tab) + mask(threadNo, TCBDIRBITS);
  tcb->tid        = tid;
  tcb->status     = Halted;
  tcb->space      = space;
  tcb->utcb       = 0;
  tcb->vutcb      = 0xffffffff;
  tcb->sendqueue  = 0;
  tcb->next       = tcb;
  tcb->prev       = tcb;
  tcb->prio       = 128;       // Default is unspecified
  tcb->scheduler  = scheduler;
  tcb->timeslice  =
  tcb->timeleft   = 10000;     // Default timeslice is 10ms
  tcb->quantleft  = 0;         // Default quantum is infinite
  initUserContext(&(tcb->context));
  enterSpace(space);           // Register the thread in this space
  return tcb;
}

/*-------------------------------------------------------------------------
 * Activate a TCB, allocating storage for the thread's UTCB area and, if
 * this is the first thread to be activated, creating the initial page
 * directory.
 */
static void activateTCB4(struct TCB* tcb) {
  tcb->utcb   = allocUtcb4(tcb->space, tcb->vutcb);
  tcb->status = Receiving(Startup); // TODO: run an IPC receive phase here?
  tcb->utcb->exceptionHandler = nilthread;
}

/*-------------------------------------------------------------------------
 * Destroy a TCB, which entails removing it from the corresponding address
 * space; deactivating it if it was active; and, if it was the last valid
 * TCB in the underlying page, deleting the entry from the tcb directory.
 */
static void destroyTCB(struct TCB* tcb) {
  // Register that a TCB has been taken out this space.
  exitSpace(tcb->space, tcb->utcb);
  tcb->space = 0; // mark as an empty TCB

  // Test to see if this creates an empty slot in the tcbDir
  TCBTable* tab = (TCBTable*)align((unsigned)tcb, 12);
  if (--tab[0]->count==0) {
DEBUG(printf("Clearing tcbDir slot\n");)
    tcbDir[(tcb->tid)>>(VERSIONBITS+TCBDIRBITS)] = 0;
    freePage(tab);
  }
DEBUG(extern void showRunqueue();)
DEBUG(showRunqueue();)
}

/*-------------------------------------------------------------------------
 * The "ThreadControl" System Call:
 *-----------------------------------------------------------------------*/
static void createThread() {
  // Phase 1: Validate parameters -----------------------------------------
  struct TCB* spaceTCB;
  if (ThreadControl_SpaceId!=ThreadControl_DestId) {   // Reuse addr space?
    if (!(spaceTCB = findTCB(ThreadControl_SpaceId))) {
      retError(ThreadControl_Result, INVALID_SPACE);
    }
  } else {
    spaceTCB = 0;
  }

  if (!isGlobal(ThreadControl_SchedulerId)) {           // Scheduler valid?
    retError(ThreadControl_Result, INVALID_SCHEDULER);
  }

  if (ThreadControl_PagerId!=nilthread) {              // Activation reqd?
    if (!findTCB(ThreadControl_SchedulerId)) {         // Scheduler exists?
      retError(ThreadControl_Result, INVALID_SCHEDULER);
    } else if (!spaceTCB                               // Space config'd?
               || !configuredSpace(spaceTCB->space)) {
      retError(ThreadControl_Result, INVALID_SPACE);
    } else if (!validUtcb(spaceTCB->space,             // Utcb loc valid?
                          ThreadControl_UtcbLocation)) {
      retError(ThreadControl_Result, INVALID_UTCB);
    } 
  }

  // Phase 2: Make changes ------------------------------------------------
  if (!availPages(6)) {                                     // Mem avail?
    retError(ThreadControl_Result, OUT_OF_MEMORY);
  } else {
    struct Space* space = spaceTCB ? spaceTCB->space : allocSpace1();
    struct TCB* tcb     = allocTCB1(ThreadControl_DestId,
                                    space,
                                    ThreadControl_SchedulerId);
    tcb->vutcb = ThreadControl_UtcbLocation;
    if (ThreadControl_PagerId!=nilthread) {
      activateTCB4(tcb);
      tcb->utcb->myGlobalId = tcb->tid;
      tcb->utcb->pager      = ThreadControl_PagerId;
      refreshSpace();   // new utcb mapping might have changed the space
    }
  }
}

static void modifyThread(struct TCB* tcb) {
  // Phase 1: Validate parameters -----------------------------------------
  unsigned vutcb = ThreadControl_UtcbLocation;
DEBUG(printf("Kernel:modify thread, vutcb=%x\n", vutcb);)
  if (vutcb==(-1)) {
    vutcb = tcb->vutcb;
DEBUG(printf("Kernel:modify thread, reset to vutcb=%x\n", vutcb);)
  }

  if (ThreadControl_PagerId!=nilthread && !tcb->utcb) { // Activate reqd
DEBUG(printf("Kernel:activate required\n");)
    if (!configuredSpace(tcb->space)) {                 // Space config'd?
DEBUG(printf("Kernel:space not configured\n");)
      retError(ThreadControl_Result, INVALID_SPACE);
    } else if (!validUtcb(tcb->space, vutcb)) {         // Valid utcb loc?
DEBUG(printf("Kernel:invalid utcb location\n");)
      retError(ThreadControl_Result, INVALID_UTCB);
    } else if (!availPages(4)) {                        // Mem available?
DEBUG(printf("Kernel:out of memory\n");)
      retError(ThreadControl_Result, OUT_OF_MEMORY);
    }
  }
  // TODO: Add support for space migration

  // Phase 2: Make changes ------------------------------------------------
DEBUG(printf("Kernel:make changes\n");)
  if (ThreadControl_SchedulerId!=nilthread) {           // Change scheduler
    tcb->scheduler = ThreadControl_SchedulerId;
  }
  if (ThreadControl_UtcbLocation!=(-1)) {               // Change utcb loc
    tcb->vutcb = vutcb;   // TODO: Should only change if thread is inactive
  }
  if (ThreadControl_PagerId!=nilthread) {               // Set/change pager
    if (!tcb->utcb) {                                   // Activate thread
DEBUG(printf("Kernel: activating thread\n");)
      activateTCB4(tcb);
    }
    tcb->utcb->pager = ThreadControl_PagerId;
  }
  tcb->tid = ThreadControl_DestId;                      // Change thread id
  if (tcb->utcb) {
     tcb->utcb->myGlobalId = tcb->tid;
  }
DEBUG(printf("Kernel: DONE modify thread\n");)
}

static void deleteThread(struct TCB* tcb) {
DEBUG(printf("Kernel: deleteThread tcb=%x, tid=%x\n", tcb, tcb->tid);)
  if (isSending(tcb)) {
DEBUG(printf("Thread %x was blocked waiting to send\n", tcb->tid);)
    // If this thread was blocked waiting to send, remove it from the
    // receiver's queue:
    struct TCB* recv = tcb->receiver;
    recv->sendqueue = removeTCB(recv->sendqueue, tcb);
  }
DEBUG(printf("clearing any threads waiting to send to %x\n", tcb->tid);)
  while (tcb->sendqueue) {
    // If any other threads were blocked waiting to send to this
    // one, then abort them and remove them from the send queue:
    struct TCB* send = tcb->sendqueue;
    tcb->sendqueue = removeTCB(tcb->sendqueue, send);
    sendError(ipctype(send), send, NonExistingPartner);
  }
  ThreadControl_Result = 1;
DEBUG(printf("halting thread %x\n", tcb->tid);)
  haltThread(tcb);
DEBUG(printf("destroy tcb %x\n", tcb->tid);)
  destroyTCB(tcb);
DEBUG(printf("reschedule!\n", tcb->tid);)
  reschedule();   // (just in case tcb was current or timeslice holder...)
}

static void interruptControl(ThreadId irqId) {
  unsigned n = threadNo(irqId);
  if (n>=NUMIRQs || mask(irqId, VERSIONBITS)!=1) {
    retError(ThreadControl_Result, INVALID_THREADID);
  } else if (ThreadControl_SpaceId!=irqId) {
    retError(ThreadControl_Result, INVALID_SPACE);
  } else if (ThreadControl_UtcbLocation!=(-1)) {
    retError(ThreadControl_Result, INVALID_UTCB);
  } else if (ThreadControl_SchedulerId!=nilthread) {
    retError(ThreadControl_Result, INVALID_SCHEDULER);
  } else {
    ThreadId    pagerId = ThreadControl_PagerId;
    struct TCB* irqTCB  = existsTCB(n);
    if (pagerId!=nilthread && pagerId!=(ThreadId)(irqTCB->vutcb)) {
      // Changing the handler for an interrupt, so halt the interrupt thread:
      ASSERT(irqTCB->status!=Runnable, "an interrupt thread is runnable");
      if (isSending(irqTCB)) {               // Abort a waiting send
        struct TCB* recv = irqTCB->receiver; // remove from send queue
        recv->sendqueue  = removeTCB(recv->sendqueue, irqTCB);
      }
      irqTCB->status = Halted;

      // Enable or disable interrupt according to "pager" setting:
      if ((irqTCB->vutcb=(unsigned)pagerId)==irqId) { // disable interrupt
        disableIRQ(n);
      } else {                                        // enable interrupt
        enableIRQ(n);
      }
    }
  }
}

ENTRY threadControl() {
DEBUG(printf("Kernel:threadControl %x %x %x %x %x\n", ThreadControl_DestId, ThreadControl_SpaceId, ThreadControl_SchedulerId, ThreadControl_PagerId, ThreadControl_UtcbLocation);)
  if (!privileged(current->space)) {         // check for privileged thread
DEBUG(printf("Kernel:threadControl:not privileged\n");)
    retError(ThreadControl_Result, NO_PRIVILEGE);
  } else {
    ThreadId destId = ThreadControl_DestId;
    unsigned destNo = threadNo(destId);
DEBUG(printf("Kernel:threadControl:privileged, destId=%x, destNo=%x\n",destId,destNo);)
    if (destNo<SYSTEMBASE) {                            // interrupt thread?
DEBUG(printf("Kernel:threadControl:interrupt thread\n");)
      interruptControl(destId);
    } else if (destNo<USERBASE || !isGlobal(destId)) {  // invalid id?
DEBUG(printf("Kernel:threadControl:invalid id\n");)
      retError(ThreadControl_Result, INVALID_THREADID);
    } else {                                            // regular thread?
      struct TCB* dest = existsTCB(destNo);
      if (!dest) {                                      // create thread?
DEBUG(printf("Kernel:threadControl:create thread\n");)
        createThread();
      } else if (ThreadControl_SpaceId==nilthread) {    // delete thread?
DEBUG(printf("Kernel:threadControl:delete thread\n");)
        deleteThread(dest);
      } else {                                          // modify thread?
DEBUG(printf("Kernel:threadControl:modify thread\n");)
        modifyThread(dest);
      }
    }
  }
DEBUG(printf("Kernel:threadControl:done\n");)
  ThreadControl_Result = 1;
  resume();
}

/*-------------------------------------------------------------------------
 * The "SpaceControl" System Call:
 *-----------------------------------------------------------------------*/
ENTRY spaceControl() {
  if (!privileged(current->space)) {    /* check for privileged thread   */
    retError(SpaceControl_Result, NO_PRIVILEGE);
  } else {
    struct TCB* dest = findTCB(SpaceControl_SpaceSpecifier);
    if (!dest) {
      retError(SpaceControl_Result, INVALID_SPACE);
    } else if (!activeSpace(dest->space)) { /* ignore if active threads  */
      Fpage kipArea  = SpaceControl_KipArea;
      Fpage utcbArea = SpaceControl_UtcbArea;
      unsigned kipEnd, utcbEnd;
      if (isNilpage(utcbArea)           /* validate utcb area            */
       || fpageSize(utcbArea)<MIN_UTCBAREASIZE
       || (utcbEnd=fpageEnd(utcbArea))>=KERNEL_SPACE) {
        retError(SpaceControl_Result, INVALID_UTCB);
      } else if (isNilpage(kipArea)     /* validate KIP area             */
       || fpageSize(kipArea)!=KIPAREASIZE
       || (kipEnd=fpageEnd(kipArea))>=KERNEL_SPACE
       || (kipEnd>=fpageStart(utcbArea) && utcbEnd>=fpageStart(kipArea))) {
        retError(SpaceControl_Result, INVALID_KIPAREA);
      } else {
        configureSpace(dest->space, kipArea, utcbArea);
      }
    }
    SpaceControl_Result      = 1;
    SpaceControl_Control     = 0;   /* control parameter is not used */
    resume();
  }
}

/*-------------------------------------------------------------------------
 * The "Unmap" System Call:
 *-----------------------------------------------------------------------*/
ENTRY unmap() {
  /* TODO: implement this system call. */
  resume();
}

/*-----------------------------------------------------------------------*/

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
 * Scheduling and Context Switching Functions:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "threads.h"
#include "hardware.h"

#define DEBUG(cmd)	/*cmd*/

struct TCB*        current;     // points to the current thread's TCB
static struct TCB* holder;      // points to the current timeslice holder
static struct TCB* idleTCB;     // points to the idle TCB
static struct TCB* runqueue[PRIORITIES];

/*-------------------------------------------------------------------------
 * Initialize scheduling data structures (runqueue and idle thread).
 */
void initScheduling() {
  ASSERT(PRIOBITS <= 8*sizeof(byte), "too few priority bits");
  for (unsigned prio=0; prio<PRIORITIES; prio++) {
    runqueue[prio] = 0;
  }

  // Construct idle thread: -----------------------------------------------
  abortIf(!availPages(2), "Failed to allocate idle thread");
  struct Space* idleSpace   = allocSpace1();
  ThreadId      idleTid     = threadId(SYSTEMBASE, 1);
  idleTCB                   = allocTCB1(idleTid, idleSpace, idleTid);
  idleTCB->timeslice        = 0;
  initIdleContext(&(idleTCB->context), (unsigned)halt);
}

/*-------------------------------------------------------------------------
 * Priority set: we maintain a heap that contains the priorities of all of
 * the runnable threads that are in the system at any given time.  This
 * allows us to find the highest priority in constant time.  Insertion and
 * deletion are log(p), where p is the size of the set of priorities.  We
 * choose this data structure on the basis of the following assumptions:
 *  - p is likely to be small (many threads have the same priority); and
 *  - inserts and deletes are relatively uncommon, occuring only when we
 *    add the first thread or remove the last at a given priority.
 * If you want to insert one element and delete another, you should do the
 * insert operation first to reduce the likelihood of changing the set of
 * priorities.  It will be interesting to try some experiments and
 * determine whether these prejudices hold up in practice ...
 *-----------------------------------------------------------------------*/
// Max Heap: children of i are 2i+1, 2i+2; parent of i is (i-1)/2
static unsigned prioset[PRIORITIES];    // A heap of active priorities
static unsigned prioidx[PRIORITIES];    // Index priorities in prioset
static unsigned priosetSize = 0;        // Number of entries in prioset

/*-------------------------------------------------------------------------
 * Insert "prio" into "prioset" given that (a) there is a gap at index "i";
 * and (b) the rest of the structure, excluding "i" is a valid heap.
 */
static void heapRepairUp(unsigned prio, unsigned i) {
  while (i>0) {
    unsigned parent = (i-1)>>1;
    unsigned pprio  = prioset[parent];
    if (pprio<prio) {
      prioset[i] = pprio;
      prioidx[pprio] = i;
      i = parent;
    } else {
      break;
    }
  }
  prioset[i] = prio;
  prioidx[prio] = i;
}

/*-------------------------------------------------------------------------
 * Insert "prio" into "prioset" by replacing the maximum element at "i".
 * Assumes that the left and right children of "i" (if they exist) both
 * satisfy the heap property.
 */
static void heapRepairDown(unsigned prio, unsigned i) {
  for (;;) {   // move bigger elements up until we find a place for prio
    unsigned c = 2*i+1;
    if (c+1<priosetSize) {      // two children
      if (prio>prioset[c] && prio>prioset[c+1]) {
        break;
      } else if (prioset[c+1] > prioset[c]) {
        c = c+1;
      }
    } else if (c<priosetSize) { // one child
      if (prio>prioset[c]) {
        break;
      }
    } else {                    // no children
      break;
    }
    prioset[i] = prioset[c];
    prioidx[prioset[c]] = i;
    i                   = c;
  }
  prioset[i] = prio;
  prioidx[prio] = i;
}

/*-------------------------------------------------------------------------
 * Insert an entry into a doubly linked list of TCBs.  We assume that the
 * TCB is not already included in either this or any other list.
 */
struct TCB* insertTCB(struct TCB* queue, struct TCB* tcb) {
  if (queue) {
    tcb->prev        = queue->prev;
    queue->prev      = tcb;
    tcb->prev->next  = tcb;
    return tcb->next = queue;
  } else {
    return tcb->next = tcb->prev = tcb;
  }
}

/*-------------------------------------------------------------------------
 * Remove an entry from a doubly linked list of TCBs.  We assume that the
 * TCB is initially included in this list.
 */
struct TCB* removeTCB(struct TCB* queue, struct TCB* tcb) {
  if (tcb->next == tcb) {          // tcb is the last entry in the queue
    return 0;
  } else {                         // unlink tcb from queue and update
    struct TCB* next = tcb->next;  // queue head if necessary
    struct TCB* prev = tcb->prev;
    next->prev = prev;
    prev->next = next;
    return (queue==tcb ? next : queue);
  }
}
 
/*-------------------------------------------------------------------------
 * Add a thread to the appropriate runqueue:
 */
void insertRunnable(struct TCB* tcb) {       // Pre: tcb not in runqueue
  if (runqueue[tcb->prio]==0) {
    heapRepairUp(tcb->prio, priosetSize++);
  }
  runqueue[tcb->prio] = insertTCB(runqueue[tcb->prio], tcb);
}

/*-------------------------------------------------------------------------
 * Remove a runnable process from the runqueues:
 */
void removeRunnable(struct TCB* tcb) {       // Pre: tcb is in the runqueue
  if (!(runqueue[tcb->prio] = removeTCB(runqueue[tcb->prio], tcb))) {
    unsigned rprio = prioset[--priosetSize];   // remove last entry on heap
    if (rprio!=tcb->prio) {      // we wanted to remove a different element
      unsigned i = prioidx[tcb->prio];
      heapRepairDown(rprio, i);
      heapRepairUp(prioset[i], i);
    }
    // The following is needed only if we want an O(1) membership test
    prioidx[tcb->prio] = PRIORITIES;
  }
}

/*-------------------------------------------------------------------------
 * Switch to a specific user thread.  In general, this will not be the same
 * as the most recently executed user thread, so we do not make any special
 * case for the possibility that tcb==current.
 */
static void inline switchTo(struct TCB* tcb) {
  struct Context* ctxt = &(tcb->context);
DEBUG(printf("Switching to thread %x (tcb=%x)\n", tcb->tid, tcb);)
  current  = tcb;                  // Change current thread
  *utcbptr = tcb->vutcb            // Change UTCB address
             + (unsigned)&(((struct UTCB*)0)->mr[0]); // TODO: fix this
DEBUG(printf("set utcbptr to %x\n", *utcbptr);)
  esp0     = (unsigned)(ctxt + 1); // Change esp0
DEBUG(extern void showSpace(struct Space* space);)
DEBUG(showSpace(tcb->space);)
  switchSpace(tcb->space);        // Change address space
DEBUG(printf("switched space, context is at %x\n\n", ctxt);)
  returnToContext(ctxt);
}

/*-------------------------------------------------------------------------
 * Select a new thread to execute.  We pick the next runnable thread with
 * the highest priority.
 */
void reschedule() {
  switchTo(holder = priosetSize ? runqueue[prioset[0]] : idleTCB);
}

/*-------------------------------------------------------------------------
 * Timeslice accounting:
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Calculate the next timeslice length for the specified tcb, draining the
 * quantum if it was specified.  Returns true if tcb should be scheduled
 * (i.e., if its quantum has not expired), or false otherwise.
 */
static unsigned refillHolder() {
  if (holder->quantleft==(-1)) {                     // quantum expired?
    // TODO: expire holder
    // send a preempt ipc; take holder out of the run queue; reschedule.
    return 0;
  } else if (holder->quantleft==0) {                // quantum infinite?
    holder->timeleft += holder->timeslice;
  } else if (holder->quantleft>=holder->timeslice) {// take from quantum
    holder->timeleft  += holder->timeslice;
    holder->quantleft -= holder->timeslice;
  } else {                                // prepare for final timeslice
    holder->timeleft += holder->quantleft;
    holder->quantleft = (-1);
  }
  return 1;
}

/*-------------------------------------------------------------------------
 * Timer interrupt:
 *-----------------------------------------------------------------------*/
unsigned clockTick          = 1000000/HZ;  // TODO: is this ok?
unsigned long long sysClock = 0;

ENTRY timerInterrupt() {
  maskAckIRQ(TIMERIRQ);           // Mask and acknowledge timer interrupt
  enableIRQ(TIMERIRQ);		  // TODO: can this be optimized?
  sysClock += clockTick;          // Update system clock

  if (holder->timeslice != 0) {          // finite timeslice; do accounting
    if (holder->timeleft >= clockTick) {      // timeslice not finished yet
      holder->timeleft -= clockTick;
    } else {
DEBUG(printf("TIMESLICE EXPIRED at time %d\n", (unsigned)sysClock);)
      if (refillHolder()) {                 // timeslice over; prepare next
        if (holder->timeleft > clockTick) {        // account for last tick
          holder->timeleft -= clockTick;
        }
        if (holder != holder->next) {
          runqueue[holder->prio] = holder->next;         // rotate runqueue
        }
      }
DEBUG(printf("holder->timeslice=%d, holder->timeleft=%d\n",
  holder->timeslice, holder->timeleft);)
      reschedule();                                // switch to next thread
    }
  }

  // Here if infinite timeslice or if current timeslice has not finished 
  if (priosetSize && prioset[0] > holder->prio) {
    reschedule();                     // preempt by higher priority thread?
  }
  resume();
}

/*-------------------------------------------------------------------------
 * The "SystemClock" System Call:
 *-----------------------------------------------------------------------*/
ENTRY systemClock() {
   SystemClock_Lo = (unsigned)sysClock;
   SystemClock_Hi = (unsigned)(sysClock>>32);
DEBUG(printf("sysclock %x %x\n", SystemClock_Hi, SystemClock_Lo);)
   resume();
}

/*-------------------------------------------------------------------------
 * The "ThreadSwitch" System Call:
 *-----------------------------------------------------------------------*/
ENTRY threadSwitch() {
  ThreadId destId = ThreadSwitch_GetDest;    // find destination
  if (destId!=nilthread) {
    struct TCB* dest = findTCB(destId);      // timeslice donation
    if (dest && (dest->status==Runnable)) {
      switchTo(dest);
    }
  }
  holder->timeleft = 0;      // discard remainder of timeslice
  refillHolder();            // prepare holder's next timeslice
  reschedule();              // and look for something new to run
}

/*-------------------------------------------------------------------------
 * The "Schedule" System Call:
 *-----------------------------------------------------------------------*/
ENTRY schedule() {
  struct TCB* dest;
  if (!isGlobal(Schedule_DestId)) {
    retError(Schedule_Result, INVALID_THREADID);
  } else if (!(dest=findTCB(Schedule_DestId)) || dest->utcb==0) {
    Schedule_Result = 1;
  } else if (dest->scheduler!=current->tid) {
    retError(Schedule_Result, NO_PRIVILEGE);
  } else {
    if (Schedule_Prio!=-1) {
      unsigned newPrio = mask(Schedule_Prio, PRIOBITS);
      if (newPrio>current->prio) {
        retError(Schedule_Result, INVALID_PARAMETER);
      } else if (newPrio!=dest->prio) { // If priority changed and dest
        if (dest->status == Runnable) { // is runnable, then remove from
          removeRunnable(dest);         // old queue, change priority,
          dest->prio = newPrio;         // and then put it back in the
          insertRunnable(dest);         // queue.
        } else {
          dest->prio = newPrio;         // Otherwise, just change priority
        }
      }
    }

    // Don't make changes until we have validated the params?
    // ignore processor control

    if (Schedule_TsLen!=(-1)) {
      dest->timeslice = dest->timeleft = Schedule_TsLen;
    }

    if (Schedule_TotQuantum!=(-1)) {
      // TODO: fill this in ...
    }

    Schedule_RemTs      = dest->timeleft;
    Schedule_RemQuantum = dest->quantleft;
    Schedule_Result     = isSending(dest) ? 4 :
                            isReceiving(dest) ? 6 :
                              (dest->status==Runnable) ? 3 : 2;
  }
  resume();
}

/*-------------------------------------------------------------------------
 * Display the current runqueue for the purposes of debugging.
 */
void showRunqueue() { // TODO: debugging only
  printf("*** runqueue (%d priorities in use)\n", priosetSize);
  for (int i=0; i<priosetSize; i++) {
    printf(" %d: ", prioset[i]);
    struct TCB* first = runqueue[prioset[i]];
    struct TCB* tcb   = first;
    if (tcb==0) {
      printf("ERROR this runqueue is empty!\n");
    } else {
      do {
        printf(" %x[status %x, space %x]", tcb->tid, tcb->status, tcb->space);
      } while ((tcb=tcb->next)!=first);
      printf("\n");
    }
  }
}

/*-----------------------------------------------------------------------*/

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
/* A simple root server program that we will run in user mode.
 */
#include "simpleio.h"
#include <l4/space.h>
#include <l4/thread.h>
#include <l4/schedule.h>
#include <l4/ipc.h>
#include "hardware.h"

extern unsigned readTSC(unsigned* hi);
typedef unsigned long long u64;

u64 tsc() {
  extern unsigned readTSC(unsigned* hi);
  unsigned hi;
  unsigned lo = readTSC(&hi);
  return (((u64)hi) << 32) | (u64)lo;
}

L4_ThreadId_t ping;
L4_ThreadId_t pong;

#define PINGSTACKSIZE 4096
unsigned char pingstack[PINGSTACKSIZE];

#define N  10000
#define U  60

void ping_thread() {

  int calls = 0;
  for (int p=0; p<4; p++) {
    for (int q=0; q<=U; q++) {
      u64 before = tsc();
      for (int r=0; r<N; r++) {
        L4_LoadMR(0, q);   // tag: Empty message
        L4_Call(pong);
        calls++;
      }
      u64 clock = tsc() - before;
      unsigned hi = (unsigned) (clock>>32);
      unsigned lo = (unsigned) clock;
      // run, num MRs, hi, lo ... csv format
      printf("%x, %d, %d, %d\n", p, q, hi, lo);
    }
  }
  printf("executed %d calls\n", calls);
      
#if 0  
  u64 before = tsc();
  printf("TSC:%lx\n", before);

  int i=0;
  for (; i<10; i++) {
    printf("Sending %d\n", i);
  }

  unsigned afterhi;
  unsigned afterlo = readTSC(&afterhi);
  printf("TSC %x:%x\n", afterhi, afterlo);
#endif

  L4_Call(ping);
  for (;;) {
    printf("ping is still alive!\n");
  }
}

#define PONGSTACKSIZE 4096
unsigned char pongstack[PONGSTACKSIZE];

void pong_thread() {
  L4_MsgTag_t tag = L4_Receive(ping);
  for (;;) {
//    printf("Received %d untyped words\n", L4_UntypedWords(tag));
    L4_LoadMR(0, 0);   // tag: Empty message, ping back to interrupt thread
    tag = L4_Call(ping);
  }
  for (;;) {
    printf("pong has escaped!\n");
  }
/*
  int i=0;
  for (; i<10; i++) {
    printf("pong for president %d!\n", i);
    L4_Yield();
  }
  for (;;) {
    if (++i==11123) {
      printf("pong is still alive!");
    }
  }
*/
}

#if 0
void startPing() {
  L4_ThreadId_t ping = L4_GlobalId(100,1);
  L4_ThreadId_t me   = L4_Myself();
  L4_Word_t     utcb = 0x200000;
  L4_Word_t     control;
  printf("creating ping (tid %x) %x\n", ping,
   L4_ThreadControl(ping, ping, me, L4_nilthread, (void*) (-1)));
  printf("configuring ping's space %x\n",
   L4_SpaceControl(ping, 0, L4_FpageLog2(0x100000, 12),
                            L4_FpageLog2(utcb, 12), &control));
  printf("Error code is %x\n", L4_ErrorCode());
  printf("activating ping: %x\n",
   L4_ThreadControl(ping, ping, L4_nilthread, L4_Pager(), (void*)utcb));
  printf("Pager is %x\n", L4_Pager());
  printf("Error code is %x\n", L4_ErrorCode());

  L4_Start_SpIpFlags(ping,
                     (((L4_Word_t)pingstack) + PINGSTACKSIZE),
                     ((L4_Word_t)ping_thread),
                     0);
/*
  printf("Sending startup message to ping\n");
  L4_LoadMR(0, 2);   // tag: two untyped items
  L4_LoadMR(1, (L4_Word_t)ping_thread);                 // program counter
  L4_LoadMR(2, ((L4_Word_t)pingstack) + PINGSTACKSIZE); // stack pointer
  printf("Result from send is %x\n", L4_Send(ping));
*/
}
#endif

void spawn(char* name,
           L4_ThreadId_t tid,
           L4_Word_t     utcbNo,
           L4_ThreadId_t spaceSpec,
           L4_ThreadId_t scheduler,
           L4_ThreadId_t pager,
           L4_Word_t     ip,
           L4_Word_t     sp) {

  L4_Word_t utcb = 0x200000;

  printf("spawning %s: tid=%x, space=%x, scheduler=%x, pager=%x\n",
          name, tid, spaceSpec, scheduler, pager);

  printf("created %s (tid %x) %x\n", name, tid,
   L4_ThreadControl(tid, spaceSpec, scheduler, L4_nilthread, (void*) (-1)));

//printf("SPAWNING %s: tid=%x, space=%x, scheduler=%x, pager=%x\n",
//        name, tid, spaceSpec, scheduler, pager);

  if (tid.raw==spaceSpec.raw) {
    L4_Word_t control;
    printf("configured space for %s -> %x\n", name,
     L4_SpaceControl(tid, 0, L4_FpageLog2(0x100000, 12),
                             L4_FpageLog2(utcb, 12), &control));
    printf("Error code is %x, control=%x\n", L4_ErrorCode(), control);
  } else {
    printf("Different address space, tid=%x, spaceSpec=%x\n", tid, spaceSpec);
  }

  printf("activated %s: %x\n", name,
   L4_ThreadControl(tid, spaceSpec, L4_nilthread, pager,
                    (void*)(utcb + utcbNo*512)));
  printf("Pager is %x\n", pager);
  printf("Error code is %x\n", L4_ErrorCode());

  printf("starting %s: ip=%x, sp=%x\n", name, ip, sp);
  L4_Start_SpIpFlags(tid, sp, ip, 0);
  printf("thread %s (tid %x) is now running\n", name, tid);
}

void cmain() {
  setWindow(10, 14, 41, 39);
  setAttr(0x5);
  cls();
  printf("This is a root server!\n");

  extern void showKIP();
  showKIP();

  ping = L4_GlobalId(100,1);
  pong = L4_GlobalId(200,1);

  //startPing();
  spawn("ping", ping,                            // Name & thread id
        0, ping,                                 // utcbNo & space spec
        L4_Myself(),                             // Scheduler
        L4_Pager(),                              // Pager
        (L4_Word_t)ping_thread,                  // instruction pointer
        ((L4_Word_t)pingstack) + PINGSTACKSIZE); // stack pointer

  //startPong();
  spawn("pong", pong,                            // Name & thread id
        0, pong,                                 // utcbNo & space spec
        L4_Myself(),                             // Scheduler
        L4_Pager(),                              // Pager
        (L4_Word_t)pong_thread,                  // instruction pointer
        ((L4_Word_t)pongstack) + PONGSTACKSIZE); // stack pointer

  //keyboard listener
  L4_ThreadId_t keyId  = L4_GlobalId(1, 1);  // Keyboard on IRQ1
  L4_ThreadId_t rootId = L4_MyGlobalId();    // My id

  printf("keyboard id = %x, my id = %x\n", keyId, rootId);
  printf("associate produces %x\n",
         L4_AssociateInterrupt(keyId, rootId));

  L4_MsgTag_t tag = L4_Receive(keyId);
  for (;;) {
    printf("received msg (tag=%x) from %x\n", tag, keyId);
    if (L4_IpcSucceeded(tag)   &&
        L4_UntypedWords(tag)==0 &&
        L4_TypedWords(tag)  ==0) {
      printf("Scancode = 0x%x\n", inb(0x60));
      L4_LoadMR(0, 0);   // tag: Empty message, ping back to interrupt thread
      tag = L4_Call(keyId);
      printf("root's Call completed ...\n");
    } else {
      printf("Ignoring message/failure, trying again ...\n");
      tag = L4_Receive(keyId);
    }
  }
  printf("This message won't appear!\n");
}

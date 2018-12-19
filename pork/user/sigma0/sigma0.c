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
/* A simple program that we will run in user mode.
 */
#include "simpleio.h"
#include "l4.h"
#include <l4/thread.h>
#include <l4/ipc.h>
#include "kip.h"

void cmain() {
  setWindow(10, 14, 0, 39);
  setAttr(0x3);
  cls();
  printf("This is a simple sigma0 server\n");
  unsigned myutcb;
  asm(" movl %%gs:0, %0\n" : "=r"(myutcb));
  printf("sigma0 utcb is at: %x\n", myutcb);

  L4_ThreadId_t from;
  printf("Now waiting for first message ...\n");
  L4_MsgTag_t   tag = L4_Wait(&from);
  printf("First message from %x\n", from);
  for (;;) {
  printf("sigma0: received msg (tag=%x) from %x\n", tag, from);
    if (L4_IpcSucceeded(tag)    &&
        L4_UntypedWords(tag)==2 &&
        L4_TypedWords(tag)  ==0 &&
        (tag.raw>>20)==0xffe) {
      L4_Word_t mr1, mr2;
      L4_StoreMR(1, &mr1);
      L4_StoreMR(2, &mr2);
    printf("pagefault %x, mr1=%x, mr2=%x\n", from, mr1, mr2);
      L4_LoadMR(0, (2<<6));   // tag: 2 words of typed items
      L4_LoadMR(1, (mr1 & ~0xfff) | 8); // MapItem
      L4_LoadMR(2, L4_FpageLog2(mr1, 12).raw | L4_FullyAccessible); 
      tag = L4_ReplyWait(from, &from);
    } else {
      printf("Ignoring message/failure, trying again ...\n");
      printf("succ=%d, u=%d, t=%d, tag = %x\n", 
             L4_IpcSucceeded(tag),
             L4_UntypedWords(tag),
             L4_TypedWords(tag),
             tag.raw);
      tag = L4_Wait(&from);
    }
  }
}

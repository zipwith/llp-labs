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
 * user.c: a simple user-level program
 * Mark P. Jones, Portland State University, May 2016
 *-----------------------------------------------------------------------*/
#include "capio.h"
#include "syscalls.h"

void kputs(unsigned cap, char* s) {
  while (*s) {
    kputc(cap, *s++);
  }
}

void cmain() {
  int i;

  // Basic tests of console and window output: ----------------------------
  cls();
  unsigned myid = kputc(CONSOLE, '!');
  printf("My process id is %x\n", myid);
  puts("in user code\n");
  for (i=0; i<4; i++) {
    kputs(CONSOLE, "hello, kernel console\n");
    puts("hello, user console\n");
    setAttr(i&0xf);
  }

  // Test operations for manipulating capability spaces: ------------------
  if (myid) {
    printf("My process id is %x\n", kputc(CONSOLE, 'A'));
    capmove(1, 4, MOVE);
    printf("My process id is %x\n", kputc(CONSOLE, 'B'));
    capmove(4, 1, COPY);
    printf("My process id is %x\n", kputc(CONSOLE, 'C'));
    capclear(1);
    printf("My process id is %x\n", kputc(CONSOLE, 'D'));
    capmove(4, 1, MOVE);
    printf("My process id is %x\n", kputc(CONSOLE, 'E'));
  } else {
    printf("I don't have a capability for the console");
  }

  // Allocate memory for this process without a capability: ---------------
  kmapPage(0x600000);
  kmapPage(0x601000);
  kmapPage(0x603000);
  unsigned stomp = 0x700000;
  for (int j=0; j<8; j++) {
    kmapPage(stomp);
    *((unsigned*)stomp) = stomp;
    stomp += (1<<12);
  }

  // Allocate memory for this process using a capability: -----------------
  unsigned memoryPool = 3;   // Capability number for untyped object
  allocPage(memoryPool,    /*slot*/12);
  allocCspace(memoryPool,  /*slot*/14);

  // Use capabilities to add memory to this address space: ----------------
  stomp = 0x80000000;        // Let's allocate a page here
  allocPageTable(memoryPool, 21/*slot*/);
  mapPageTable(21, stomp);
  mapPageTable(21, stomp+0x800000); // 8MB further
  allocPage(memoryPool,      20/*slot*/);
  mapPage(20, stomp);
  dump();

  // Loop to avoid terminating user program: ------------------------------
  puts("\n\nUser code does not return\n");
  for (;;) { /* Don't return! */
  }
  puts("This message won't appear!\n");
}

/*-----------------------------------------------------------------------*/

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
 * Kernel Initialization and Miscellaneous Functions:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "space.h"
#include "threads.h"
#include "hardware.h"

ENTRY init() {
  setVideo(KERNEL_SPACE + 0xB8000);
  setAttr(0x7f);
  cls();
  extern byte KernelBanner[];
  printf("%s\n", KernelBanner);
  setWindow(1, 8, 0, 80);
  setAttr(0xd);
  cls();

  initMemory();
  initSpaces();
  initTCBs();
  startTimer();
  reschedule();
  printf("System halting\n");  // Should be unreachable
  halt();
}

/*-------------------------------------------------------------------------
 * Abort kernel operation:
 *-----------------------------------------------------------------------*/
void abortIf(bool cond, char* msg) {
  if (cond) {
    printf("%s\n", msg);
    halt();
  }
}

/*-------------------------------------------------------------------------
 * The "ProcessorControl" System Call:
 *-----------------------------------------------------------------------*/
ENTRY processorControl() { // TODO: Put this someplace else!
  if (!privileged(current->space)) {         // check for privileged thread
    retError(ProcessorControl_Result, NO_PRIVILEGE);
  } else if (ProcessorControl_ProcessorNo!=0) {               // single CPU
    // TODO: not in spec!
    retError(ProcessorControl_Result, INVALID_PARAMETER);
  } else {
    // TODO: add rest of ProcessorControl implementation here ...
    if (ProcessorControl_InternalFreq!=(-1)) {
    }
    if (ProcessorControl_ExternalFreq!=(-1)) {
    }
    if (ProcessorControl_Voltage!=(-1)) {
    }
    ProcessorControl_Result = 1;
  }
  resume();
}

/*-------------------------------------------------------------------------
 * The "MemoryControl" System Call:
 *-----------------------------------------------------------------------*/
ENTRY memoryControl() { // TODO: Put this someplace else!
  if (!privileged(current->space)) {         // check for privileged thread
    retError(ProcessorControl_Result, NO_PRIVILEGE);
  } else {
    // TODO: Fill this in ...
  }
  resume();
}

/*-----------------------------------------------------------------------*/

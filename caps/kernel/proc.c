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
 * proc.c:
 * Mark P Jones + YOUR NAME HERE, Portland State University
 *-----------------------------------------------------------------------*/
#include "winio.h"
#include "util.h"
#include "context.h"
#include "proc.h"
#include "paging.h"
#include "caps.h"

/*-------------------------------------------------------------------------
 * User-level processes:
 */
struct Process proc[2];    // The set of all user-level processes
struct Process* current;   // Identifies the current process

void initProcess(struct Process* proc,
                 unsigned lo,
                 unsigned hi,
                 unsigned entry) {
  proc->pdir   = newUserPdir(lo, hi);
  proc->cspace = allocCspace();
  showPdir(proc->pdir);
  printf("user code is at 0x%x\n", entry);
  initContext(&proc->ctxt, entry, 0);
}

/*-------------------------------------------------------------------------
 * Switch to the next available process:
 */
void reschedule() {
  current = (current==proc) ? (proc+1) : proc;
  setPdir(toPhys(current->pdir));
}

/*-----------------------------------------------------------------------*/

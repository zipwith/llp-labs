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
 * proc.h:
 * Mark P Jones + YOUR NAME HERE, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef PROC_H
#define PROC_H
#include "context.h"

/*-------------------------------------------------------------------------
 * User-level processes:
 */
struct Process {
  struct Context ctxt;
  struct Pdir*   pdir;
  struct Cspace* cspace;
};

extern struct Process proc[];     // The set of all user-level processes
extern struct Process* current;   // Identifies the current process

extern void initProcess(struct Process* proc,
                        unsigned lo,
                        unsigned hi,
                        unsigned entry);
extern void reschedule();

#endif
/*-----------------------------------------------------------------------*/

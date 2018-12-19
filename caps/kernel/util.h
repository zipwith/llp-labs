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
 * util.h:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef UTIL_H
#define UTIL_H

/*-------------------------------------------------------------------------
 * Basic code for halting the processor, reporting a fatal error, or
 * checking an assertion:
 */
extern void halt();

static inline void fatal(char* msg) {
  printf("FATAL ERROR: %s\n", msg);
  halt();
}

static inline void assert(int cond, char* msg) {
  if (!cond) {
    printf("ASSERTION FAILED: %s\n", msg);
    halt();
  }
}

#endif
/*-----------------------------------------------------------------------*/

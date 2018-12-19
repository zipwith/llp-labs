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
 * syscalls.h:
 * Mark P. Jones, Portland State University, May 2016
 *-----------------------------------------------------------------------*/
#ifndef SYSCALLS_H
#define SYSCALLS_H

/*-------------------------------------------------------------------------
 * System calls not requiring a capability argument:
 */
extern unsigned kmapPage(unsigned addr);
extern void     dump();

/*-------------------------------------------------------------------------
 * Access to the console window:
 */
#define CONSOLE  1    // default capability slot for the console

extern unsigned kputc(unsigned cap, unsigned ch);

/*-------------------------------------------------------------------------
 * Capability space manipulation:
 */
enum Capmove { MOVE=0, COPY=1 };
extern unsigned capmove(unsigned src, unsigned dst, enum Capmove copy);
extern unsigned capclear(unsigned src);

/*-------------------------------------------------------------------------
 * Allocate memory from an untyped memory area:
 */
extern unsigned allocUntyped(unsigned ucap, unsigned slot, unsigned bits);
extern unsigned allocCspace(unsigned ucap, unsigned slot);
extern unsigned allocPage(unsigned ucap, unsigned slot);
extern unsigned allocPageTable(unsigned ucap, unsigned slot);

/*-------------------------------------------------------------------------
 * Mapping objects in to an address space:
 */
extern unsigned mapPage(unsigned cap, unsigned addr);
extern unsigned mapPageTable(unsigned cap, unsigned addr);

#endif
/*-----------------------------------------------------------------------*/

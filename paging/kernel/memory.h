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
 * memory.h:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef MEMORY_H
#define MEMORY_H

/*-------------------------------------------------------------------------
 * Memory layout:
 */
#define KB(x)             ((x)<<10)     // Number of bytes in x KB
#define MB(x)             ((x)<<20)     // Number of bytes in x MB
#define PHYSMAP           MB(32)        // Bytes physical mapped to kernel

#define fromPhys(t, addr) ((t)((unsigned)(addr)+KERNEL_SPACE))
#define toPhys(ptr)       ((unsigned)(ptr) - KERNEL_SPACE)

/*-------------------------------------------------------------------------
 * Paging constants:
 */
#define PERMS_KERNELSPACE 0x83          // present, write, supervisor, superpg
#define PERMS_USER_RO     0x05          // present,        user level
#define PERMS_USER_RW     0x07          // present, write, user level
#define PERMS_SUPERPAGE   0x80          //                             superpg

#define PAGESIZE          12
#define SUPERSIZE         22
#define PAGEMASK          ((1<<PAGESIZE)-1)

/*-------------------------------------------------------------------------
 * General bit twiddling:
 *
 * Note that maskTo and alignTo will not work for a>=32; Intel ignores
 * all but lower 5 bits of the shift argument.
 */
#define maskTo(e, a) ((e)&((1<<(a))-1)) /* extract lower a bits of e     */
#define alignTo(e,a) (((e)>>(a))<<(a))  /* clear lower a bits of e       */

// TODO (Step 2): complete the following macro definitions.  They need to
// be implemented here as macros rather than inline functions because
// this code will be #included in assembly code too, and the assembler
// doesn't understand C syntax ...

// pageStart(x) should return the address of the first byte in the page
// that contains address x.  e.g. pageStart(0x1234) = 0x1000.
//#define pageStart(x)      (... FILL ME IN ...)

// pageEnd(x) should return the address of the last byte in the page
// that contains address x.  e.g., pageEnd(0x1234) = 0x1fff.
//#define pageEnd(x)        (... FILL ME IN ...)

// pageNext(x) should return the address of the first byte in the
// page that comes immediately after the page containing x.
// e.g., pageNext(0x1234) = 0x2000.
//#define pageNext(x)       (... FILL ME IN ...)

// firstPageAfter(x) should return the address of the first page
// whose starting address is >= x.  (By "first", we mean the first
// start page address that we would come to as x is increased, i.e.,
// using the lowest possible address.)  e.g., firstPageAfter(0x1234)
// = 0x2000, but firstPageAfter(0x2000) = 0x2000.
//#define firstPageAfter(x) (... FILL ME IN ...)

// endPageBefore(x) should return the end address of the first
// page whose end address is <= x.  (This time, by "first", we
// mean the first end page address that we would come to as x is
// decreased, i.e., using the highest possible address.)
// e.g., endPageBefore(0x1234) = 0xfff, but endPageBefore(0x1fff)
// = 0x1fff.
//#define endPageBefore(x)  (... FILL ME IN ...)

#endif
/*-----------------------------------------------------------------------*/

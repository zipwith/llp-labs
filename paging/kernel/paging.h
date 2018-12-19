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
 * paging.h:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef PAGING_H
#define PAGING_H

/*-------------------------------------------------------------------------
 * Paging Structures:
 */
struct Pdir { unsigned pde[1024]; };
struct Ptab { unsigned pte[1024]; };

extern struct Pdir* allocPdir();
extern struct Ptab* allocPtab();

/*-------------------------------------------------------------------------
 * Set the page directory control register to a specific value.
 */
static inline void setPdir(unsigned pdir) {
  asm("  movl  %0, %%cr3\n" : : "r"(pdir));
}

/*-------------------------------------------------------------------------
 * Map a virtual address to a specific physical address:
 */
extern void mapPage(struct Pdir* pdir, unsigned virt, unsigned phys);

/*-------------------------------------------------------------------------
 * Display a description of a page directory (for debugging purposes):
 */
extern void showPdir(struct Pdir* pdir);

#endif
/*-----------------------------------------------------------------------*/

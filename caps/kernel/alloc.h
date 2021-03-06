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
 * alloc.h: Interface to capability based memory allocator
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef ALLOC_H
#define ALLOC_H

/*-------------------------------------------------------------------------
 * Exported functions:
 */
extern void      initMemory(unsigned* hdrs, unsigned* mmap);
extern void      showUntyped();
extern unsigned* allocPage();
extern unsigned  copyPage(unsigned phys);

#endif
/*-----------------------------------------------------------------------*/

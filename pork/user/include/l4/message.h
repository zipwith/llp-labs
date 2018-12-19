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
#ifndef L4_MESSAGE_H
#define L4_MESSAGE_H
#include <l4/types.h>
#include <l4/space.h>
#include <l4/utcb.h>

/* Message Registers: ---------------------------------------------------*/

static inline void L4_StoreMR(int i, L4_Word_t* w) {
  *w = (L4_GetUtcb())[i];
}

static inline void L4_StoreMRs(int i, int k, L4_Word_t* w) {
  for (L4_Word_t* v = L4_GetUtcb() + i; 0<k--; ) {
    *w++ = *v++;
  }
}

static inline void L4_LoadMR(int i, L4_Word_t w) {
  (L4_GetUtcb())[i] = w;
}

static inline void L4_LoadMRs(int i, int k, L4_Word_t* w) {
  for (L4_Word_t* v = L4_GetUtcb() + i; 0<k--; ) {
    *v++ = *w++;
  }
}

#endif

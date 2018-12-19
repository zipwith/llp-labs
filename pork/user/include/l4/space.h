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
#ifndef L4_SPACE_H
#define L4_SPACE_H
#include <l4/types.h>
#include <l4/thread.h>

EXTERNC(L4_Word_t L4_SpaceControl(
			L4_ThreadId_t spaceSpec,
			L4_Word_t control,
			L4_Fpage_t kipArea,
			L4_Fpage_t utcbArea,
			L4_Word_t* oldControl))

#if defined(__cplusplus)
/* This is a hack to "handle" the alternative version of
   SpaceControl that includes an additional redirector
   parameter.  (Except that, for the time being, we don't
   really handle it all; we just discard the extra parameter.)
 */
static inline L4_Word_t L4_SpaceControl(
			  L4_ThreadId_t spaceSpec,
			  L4_Word_t control,
			  L4_Fpage_t kipArea,
			  L4_Fpage_t utcbArea,
                          L4_ThreadId_t redirector,
			  L4_Word_t* oldControl) {
  return L4_SpaceControl(spaceSpec, control, kipArea, utcbArea, oldControl);
}

#endif

#endif

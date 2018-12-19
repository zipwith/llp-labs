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
 * capio.h: A version of the simpleio library using capabilities.
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef CAPIO_H
#define CAPIO_H

// General operations that allow us to specify a window capability.
extern void capsetAttr(unsigned cap, int a);
extern void capcls(unsigned cap);
extern void capputchar(unsigned cap, int c);
extern void capputs(unsigned cap, char* s);
extern void capprintf(unsigned cap, const char *format, ...);

// By default, we assume that our window capability is in slot 2.
#define DEFAULT_WINDOW_CAP  2

#define setAttr(a)         capsetAttr(DEFAULT_WINDOW_CAP, a)
#define cls()              capcls(DEFAULT_WINDOW_CAP)
#define putchar(c)         capputchar(DEFAULT_WINDOW_CAP, c)
#define puts(s)            capputs(DEFAULT_WINDOW_CAP, s)
#define printf(args...)    capprintf(DEFAULT_WINDOW_CAP, args)

#endif
/*-----------------------------------------------------------------------*/

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
#ifndef SIMPLEIO_H
#define SIMPLEIO_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void setVideo(unsigned);
extern void setWindow(int t, int h, int l, int w);
extern void setAttr(int a);
extern void cls(void);
extern void putchar(int c);
extern void printf(const char *format, ...);
extern int  getc(void);

#if defined(__cplusplus)
}
#endif

#endif

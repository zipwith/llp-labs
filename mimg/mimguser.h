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

#define NOENTRY  0xffffffff /* Used to signal a missing entry point      */

/* ------------------------------------------------------------------------
 * A BOOTDATA section starts with four pointers:
 * - unsigned* hdrs   points to an array of header information
 * - unsigned* mmap   points to an array of memory map information
 * - char*     cmd    loader command line string
 * - char*     str    boot module command line string
 * --------------------------------------------------------------------- */

struct BootData {
  unsigned* headers;
  unsigned* mmap;
  char*     cmdline;
  char*     imgline;
};

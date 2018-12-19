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
/* videodisp: simulate the display of a simple video frame
 * on a terminal that has support for ASCII escape code
 * sequences.  Support for blinking text is intentionally
 * not implemented.
 */
#include <stdio.h>

extern char video[25][80][2];

static void horizontal() {
    int i;
    for (i=0; i<80; i++) {
        putchar('-');
    }
    putchar('\n');
}

// Uses ANSI escape code sequences to generate appropriate colors
// (See http://en.wikipedia.org/wiki/ANSI_escape_code#Colors)
void display() {
    static char colorMap[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    int row, col;
    horizontal();
    for (row=0; row<25; row++) {
        int lastAttr = 127;
        for (col=0; col<80; col++) {
            if (video[row][col][1] != lastAttr) {
                lastAttr = video[row][col][1];
                printf("\x1b[%s;3%d;4%dm",
                       ((lastAttr & 8) ? "1" : "0"),
                       colorMap[lastAttr & 7],
                       colorMap[(lastAttr >> 4) & 7]);
            }
            int c = video[row][col][0] & 255;
            putchar(c<32 ? ' ' : c);
        }
        puts("\x1b[39;49;0m");
    }
    horizontal();
}


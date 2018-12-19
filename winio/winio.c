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
 * winio.c:  A version of the simple IO library that stores information
 * about the window that is being using in a structure rather than a set
 * of global variables.  This allows a single program to use multiple
 * windows simultaneously.
 */

#include "winio.h"

#define COLUMNS         80
#define LINES           25
#define ATTRIBUTE       7
#define VIDEO           0xB8000

typedef unsigned char single[2];
typedef single        row[COLUMNS];
typedef row           screen[LINES];

static screen* video = (screen*)VIDEO;

struct Window console[] = {
  {0, LINES, 0, COLUMNS,   // full screen
   0, 0,                   // initial position
   ATTRIBUTE}              // video attribute
};

/*-------------------------------------------------------------------------
 * Set a new start address for the video display.  If the video RAM is
 * mapped in kernel space, then the kernel should use that address for
 * output, avoiding the need for video RAM mappings in individual user
 * address spaces.
 */
void setVideo(unsigned v) {
  video = (screen*)v;
}

/*-------------------------------------------------------------------------
 * Set portion of video display to use for output.  Inputs specify the
 * (zero-based) coordinate of the top-left character and the height and
 * width of the display window.
 */
void wsetWindow(struct Window* win, int t, int h, int l, int w) {
  if (0<=t && 0<h && (t+h)<=LINES && 0<=l && 0<w && (l+w)<=COLUMNS) {
    win->top    = t;
    win->bottom = t+h;
    win->left   = l;
    win->right  = l+w;
    win->ypos   = win->top;
    win->xpos   = win->left;
  }
}

/*-------------------------------------------------------------------------
 * Set the video color attribute:
 */
void wsetAttr(struct Window* win, int a) {
  win->attr = a;
}

/*-------------------------------------------------------------------------
 * Clear the output window.
 */
void wcls(struct Window* win) {
    for (int i=win->top; i<win->bottom; ++i) {
      for (int j=win->left; j<win->right; ++j) {
        (*video)[i][j][0] = ' ';
        (*video)[i][j][1] = win->attr;
      }
    }
    win->ypos = win->top;
    win->xpos = win->left;
}

/*-------------------------------------------------------------------------
 * Output a single character.
 */
void wputchar(struct Window* win, int c) {
    extern void serial_putc(int);
    serial_putc(c);
    if (c!='\n' && c!='\r') {
        (*video)[win->ypos][win->xpos][0] = c & 0xFF;
        (*video)[win->ypos][win->xpos][1] = win->attr;
        if (++win->xpos < win->right) {
            return;
        }
    }

    win->xpos = win->left;            // Perform a newline
    if (++win->ypos >= win->bottom) { // scroll up top lines of screen ... 
        win->ypos = win->bottom-1;
        for (int i=win->top; i<win->ypos; ++i) {
          for (int j=win->left; j<win->right; ++j) {
            (*video)[i][j][0] = (*video)[i+1][j][0];
            (*video)[i][j][1] = (*video)[i+1][j][1];
          }
        }
        for (int j=win->left; j<win->right; ++j) { // fill in new blank line
          (*video)[win->ypos][j][0] = ' ';
          (*video)[win->ypos][j][1] = win->attr;
        }
    }
}

/*-------------------------------------------------------------------------
 * Output a string of characters.
 */
void wputs(struct Window* win, char* msg) {
    while (*msg) {
        wputchar(win, *msg++);
    }
}

/*-------------------------------------------------------------------------
 * Convert an integer into a string.  If base == 'x', then the string
 * is printed in hex; if base == 'd', then the string is printed as a
 * signed integer; otherwise an unsigned integer is assumed.
 */
static void itoa(char* buf, int base, long d) {
    char* p  = buf;
    char* p1;
    char* p2;
    unsigned long ud = d;
    int divisor = 10;

    if (base=='d' && d<0) {
        *p++ = '-';
        buf++;
        ud = -d;
    } else if (base=='x') {
        divisor = 16;
    }

    do {
        int remainder = ud % divisor;
        *p++ = ((remainder < 10) ? '0' : ('a' - 10)) + remainder;
    } while (ud /= divisor);

    *p = 0;

    p1 = buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1      = *p2;
        *p2      = tmp;
        p1++;
        p2--;
    }
}

/*-------------------------------------------------------------------------
 * Simple version of printf that displays a string with (optional)
 * embedded placeholders for numeric and/or string data.
 */
void wprintf(struct Window* win, const char *format, ...) {
    char** arg = (char**) &format;
    int    c;
    char   buf[20];

    arg++;
    while ((c = *format++) != 0) {
        if (c != '%') {
            wputchar(win, c);
        } else {
            char* p;
            int padChar  = ' ';
            int padWidth = 0;
            int longArg  = 0;
            int sizeArg  = 0;

            c = *format++;
            if (c == '0') {
                padChar = '0';
                c = *format++;
            }

            if (c>='0' && c<='9') {
              do {
                padWidth = 10 * padWidth + (c - '0');
                c = *format++;
              } while (c>='0' && c<='9');
            }

            if (c == 'l') {
                longArg = 1;
                c = *format++;
            }

            if (c == 'z') {
                sizeArg = 1;
                c = *format++;
            }

            switch (c) {
                case 'd' :
                case 'u' :
                case 'x' : {
                    long num = longArg ? *((long*)arg++)
                                       : (long)(*((int*)arg++));
                    char sfx = 'B';
                    if (sizeArg && num) {
                      if (((num>>30)<<30)==num) {
                        num >>= 30;
                        sfx   = 'G';
                      } else if (((num>>20)<<20)==num) {
                        num >>= 20;
                        sfx   = 'M';
                      } else if (((num>>10)<<10)==num) {
                        num >>= 10;
                        sfx   = 'K';
                      }
                    }
                    itoa(buf, c, num);
                    for (p = buf; *p; p++) {
                      padWidth--;
                    }
                    while (0<padWidth--) {
                        wputchar(win, padChar);
                    }
                    for (p = buf; *p; p++) {
                        wputchar(win, *p);
                    }
                    if (sizeArg && sfx!='B') {
                        wputchar(win, sfx);
                    }
                    break;
                }

		case 'c' :
		    wputchar(win, *((char*) arg++));
		    break;

                case 's' :
                    p = *arg++;
                    if (!p) {
                        p = "(null)";
                    }
                    while (*p) {
                        wputchar(win, *p++);
                    }
                    break;

                default :
                    wputchar(win, *((int *) arg++));
                    break;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

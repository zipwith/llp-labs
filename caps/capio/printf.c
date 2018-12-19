/*-------------------------------------------------------------------------
 * printf.c:  An implementation of capprintf, derived from code in the
 * multiboot spec.
 */

#include "capio.h"

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
void capprintf(unsigned cap, const char *format, ...) {
    char** arg = (char**) &format;
    int    c;
    char   buf[20];

    arg++;
    while ((c = *format++) != 0) {
        if (c != '%') {
            capputchar(cap, c);
        } else {
            char* p;
            int padChar  = ' ';
            int padWidth = 0;
            int longArg  = 0;

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

            switch (c) {
                case 'd' :
                case 'u' :
                case 'x' :
                    if (longArg) {
                      itoa(buf, c, *((long*)arg++));
                    } else {
                      itoa(buf, c, (long)(*((int*)arg++)));
                    }
                    for (p = buf; *p; p++) {
                      padWidth--;
                    }
                    while (0<padWidth--) {
                        capputchar(cap, padChar);
                    }
                    for (p = buf; *p; p++) {
                        capputchar(cap, *p);
                    }
                    break;

		case 'c' :
		    capputchar(cap, *((char*) arg++));
		    break;

                case 's' :
                    p = *arg++;
                    if (!p) {
                        p = "(null)";
                    }
                    while (*p) {
                        capputchar(cap, *p++);
                    }
                    break;

                default :
                    capputchar(cap, *((int *) arg++));
                    break;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

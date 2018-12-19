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
/* Compile using: gcc -o twiddletests twiddletests.c 
 *
 * Expected output:
 *
 *   pageStart(0x1234)      = 0x1000
 *   firstPageAfter(0x1234) = 0x2000
 *   endPageBefore(0x1234)  = 0xfff
 *   pageEnd(0x1234)        = 0x1fff
 *   pageStart(0x2000)      = 0x2000
 *   firstPageAfter(0x2000) = 0x2000
 *   endPageBefore(0x2000)  = 0x1fff
 *   pageEnd(0x2000)        = 0x2fff
 */
#include <stdio.h>
#include "memory.h"

void test(unsigned val) {
    printf("pageStart(0x%x)      = 0x%x\n", val, pageStart(val));
    printf("firstPageAfter(0x%x) = 0x%x\n", val, firstPageAfter(val));
    printf("endPageBefore(0x%x)  = 0x%x\n", val, endPageBefore(val));
    printf("pageEnd(0x%x)        = 0x%x\n", val, pageEnd(val));
}

int main(int argc, char** argv) {
    test(0x1234);
    test(0x2000);
}


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
#include "video.h"

int main(int argc, char** argv) {
    int i, j;
    cls();
    video[2][38][0] = 'h';  video[2][40][0] = 'i';  video[2][42][0] = '!';
    video[2][38][1] = 0x51; video[2][40][1] = 0x43; video[2][42][1] = 0x35;

    setAttr(7);
    outc('h');
    outc('i');

    display();
    return 0;
}


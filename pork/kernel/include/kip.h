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
 * Kernel Information Page:  Definitions for system constants that can be
 * #included in both .S and .c files.
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef KIP_H
#define KIP_H

#define RESERVED          0

#define REALPHYSMAP       (512<<20)     // Physical mapped to kernel
#define PHYSMAP           (32<<20)      // Physical mapped to kernel
#define UTCBPTR           0xfffff000    // Virtual address for utcb pointer

#define PERMS_KERNELSPACE 0x83          // present, write, supervisor, superpg
#define PERMS_USER_RO     0x05          // present,        user level
#define PERMS_USER_RW     0x07          // present, write, user level
#define PERMS_SUPERPAGE   0x80          //                             superpg

#define NUMIRQs           16
#define TIMERIRQ          0             // IRQ number for the system timer
#define HZ                100           // Frequency of timer interrupts

#define PAGESIZE          12
#define SUPERSIZE         22

#define R                 (4)
#define W                 (2)
#define X                 (1)

#define API_VERSION       0x87010000    // version 0x87, subversion 0x01
#define API_FLAGS         0             // little endian, 32 bit
#define KERNEL_ID         0x00000000    // id | subid | 00 | 00
#define MAX_MEMDESC       128           // max of 128 memory descriptors

#define USERBASE          48            // First user thread id
#define SYSTEMBASE        NUMIRQs       // First kernel thread id
#define THREADBITS        17            // Number of bits in thread id
#define VERSIONBITS       14            // Number of bits in thread ver
#define PRIOBITS          8             // Priorities are 8 bit values
#define PRIORITIES        (1<<PRIOBITS) // Total number of priorities
#define PRIV_UTCBADDR     0x100000      // Utcb address for privileged spaces
#define PRIV_KIPADDR      0x108000      // Kip address for privileged spaces

#define NUMMRS            64            // Maximum # of message registers
#define KIPAREASIZE       PAGESIZE      // Kip occupies one page ...
#define MIN_UTCBAREASIZE  PAGESIZE      // UTCB area must be >= one page
#define UTCBSIZE          9             // UTCB must fit in 512 bytes
#define UTCBALIGN         9             // and be aligned on 512 bytes

#define INT_THREADCONTROL 0x70          // Interrupt numbers for syscalls
#define INT_SPACECONTROL  0x71
#define INT_IPC           0x72
#define INT_EXCHANGEREGS  0x73
#define INT_SCHEDULE      0x74
#define INT_THREADSWITCH  0x75
#define INT_UNMAP         0x76
#define INT_PROCCONTROL   0x77
#define INT_MEMCONTROL    0x78
#define INT_SYSTEMCLOCK   0x79

#endif
/*-----------------------------------------------------------------------*/

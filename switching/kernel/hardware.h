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
 * hardware.h:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef HARDWARE_H
#define HARDWARE_H

typedef unsigned char byte;

static inline void outb(short port, byte b) {
  asm volatile("outb  %1, %0\n" : : "dN"(port), "a"(b));
}

static inline byte inb(short port) {
  unsigned char b;
  asm volatile("inb %1, %0\n" : "=a"(b) : "dN"(port));
  return b;
}

static inline void enableIRQ(byte irq) {
  if (irq&8) {
    outb(0xa1, ~(1<<(irq&7)) & inb(0xa1));
  } else {
    outb(0x21, ~(1<<(irq&7)) & inb(0x21));
  }
}

static inline void disableIRQ(byte irq) {
  if (irq&8) {
    outb(0xa1, (1<<(irq&7)) | inb(0xa1));
  } else {
    outb(0x21, (1<<(irq&7)) | inb(0x21));
  }
}

static inline void maskAckIRQ(byte irq) {
  if (irq&8) {
    outb(0xa1, (1<<(irq&7)) | inb(0xa1));
    outb(0xa0, 0x60|(irq&7)); // EOI to PIC2
    outb(0x20, 0x62);         // EOI for IRQ2 on PIC1
  } else {
    outb(0x21, (1<<(irq&7)) | inb(0x21));
    outb(0x20, 0x60|(irq&7)); // EOI to PIC1
  }
}

#define HZ            100           // Frequency of timer interrupts
#define PIT_INTERVAL  ((1193182 + (HZ/2)) / HZ)
#define TIMERIRQ      0

static inline void startTimer() {
  // NOTE: some sources suggest that we should include a delay after each
  // of the following outb instructions.
  outb(0x43, 0x34);  // PIT control (0x43), counter 0, 2 bytes, mode 2, binary
  outb(0x40, PIT_INTERVAL        & 0xff);  // counter 0, lsb
  outb(0x40, (PIT_INTERVAL >> 8) & 0xff);  // counter 0, msb
  enableIRQ(TIMERIRQ);
}

#endif
/*-----------------------------------------------------------------------*/

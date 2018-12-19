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
 * caps.h: Capability data structures and operations
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef CAPS_H
#define CAPS_H
#include "memory.h"
#include "alloc.h"
#include "paging.h"

/*-------------------------------------------------------------------------
 * Basic capability data structures.
 */
enum Captype {
  NullCap      = 0,
  ConsoleCap   = 1,
  WindowCap    = 2,
  CspaceCap    = 3,
  UntypedCap   = 4,
  PageCap      = 5,
  PageTableCap = 6
};

struct Cap {
  enum Captype type;
  unsigned     data[3];
};

static inline unsigned isNullCap(struct Cap* cap) {
  return cap->type==NullCap;
}

static inline void nullCap(struct Cap* cap) {
  cap->type = NullCap;
}

static inline void moveCap(struct Cap* src, struct Cap* dst, unsigned copy) {
  dst->type    = src->type;
  dst->data[0] = src->data[0];
  dst->data[1] = src->data[1];
  dst->data[2] = src->data[2];
  if (copy==0) {
    nullCap(src);
  }
}

extern void showCap(struct Cap* cap); // for debugging

/*-------------------------------------------------------------------------
 * Capability to the console window:
 */
struct ConsoleCap {
  enum Captype type;      // ConsoleCap
  unsigned     attr;      // attribute for display
  unsigned     unused[2];
};

static inline struct ConsoleCap* isConsoleCap(struct Cap* cap) {
  return (cap->type==ConsoleCap) ? (struct ConsoleCap*)cap : 0;
}

static inline void consoleCap(struct Cap* cap, unsigned attr) {
  struct ConsoleCap* ccap = (struct ConsoleCap*)cap;
  printf("Setting console cap at %x\n", ccap);
  ccap->type = ConsoleCap;
  ccap->attr = attr;
}

/*-------------------------------------------------------------------------
 * Capability to a window:
 */
struct WindowCap {
  enum Captype   type;      // WindowCap
  struct Window* window;    // Pointer to the window
  unsigned       perms;     // Permissions (CAN_{cls,setAttr,putchar})
  unsigned       unused[1];
};

#define CAN_cls      0x4    // confers permission to clear screen
#define CAN_setAttr  0x2    // confers permission to set attribute
#define CAN_putchar  0x1    // confers permission to putchar

static inline struct WindowCap* isWindowCap(struct Cap* cap) {
  return (cap->type==WindowCap) ? (struct WindowCap*)cap : 0;
}

static inline
struct WindowCap* windowCap(struct Cap* cap,
                            struct Window* window,
                            unsigned perms) {
  struct WindowCap* wcap = (struct WindowCap*)cap;
  printf("cap at %x for window %x\n", wcap, window);
  wcap->type   = WindowCap;
  wcap->window = window;
  wcap->perms  = perms;
  return wcap;
}

/*-------------------------------------------------------------------------
 * Capability space data structures:
 */
#define CSPACEBITS 8
#define CSPACESIZE (1 << CSPACEBITS)

struct Cspace {
  struct Cap caps[CSPACESIZE];
};

extern void showCspace(struct Cspace* cspace); // for debugging

static inline struct Cspace* allocCspace() {
  return (struct Cspace*)allocPage();
}

struct CspaceCap {
  enum Captype   type;      // CspaceCap
  struct Cspace* cspace;    // Pointer to the cspace
  unsigned       unused[2];
};

static inline struct Cspace* isCspaceCap(struct Cap* cap) {
  return (cap->type==CspaceCap) ? ((struct CspaceCap*)cap)->cspace : 0;
}

static inline
struct CspaceCap* cspaceCap(struct Cap* cap, struct Cspace* cspace) {
  struct CspaceCap* ccap = (struct CspaceCap*)cap;
  ccap->type   = CspaceCap;
  ccap->cspace = cspace;
  return ccap;
}

static inline void cspaceLoop(struct Cspace* cspace, unsigned w) {
   cspaceCap(cspace->caps + w, cspace);
}

typedef unsigned Cptr;      // identifies  a slot in a cspace [0..CSPACESIZE)

static inline Cptr index(unsigned w) {
  return maskTo(w >> CSPACEBITS, CSPACEBITS);
}

static inline Cptr cptr(unsigned w) {
  return maskTo(w, CSPACEBITS);
}

/*-------------------------------------------------------------------------
 * Capability to Untyped memory
 */
struct UntypedCap {
  enum Captype type;     // UntypedCap
  void*        memory;   // pointer to an fpage of size bits
  unsigned     bits;     // log2 of size in bytes
  unsigned     next;     // offset to next free location within fpage
};

static inline struct UntypedCap* isUntypedCap(struct Cap* cap) {
  return (cap->type==UntypedCap) ? (struct UntypedCap*)cap : 0;
}

static inline
struct UntypedCap* untypedCap(struct Cap* cap, void* memory, unsigned bits) {
  struct UntypedCap* ucap = (struct UntypedCap*)cap;
  printf("cap at %x for untyped memory [%x-%x]\n",
           ucap, memory, memory+(1<<bits)-1);
  ucap->type   = UntypedCap;
  ucap->memory = memory;
  ucap->bits   = bits;
  ucap->next   = 0;
  return ucap;
}

#define MIN_UNTYPED_BITS  10            // smallest legal value for bits
#define MAX_UNTYPED_BITS  (WORDSIZE-1)  // largest legal value for bits

static inline unsigned validUntypedSize(unsigned bits) {
  return (MIN_UNTYPED_BITS <= bits) && (bits <= MAX_UNTYPED_BITS);
}

extern void* alloc(struct UntypedCap* ucap, unsigned bits);
extern void  donateUntyped(struct Cap* cap, unsigned i);

/*-------------------------------------------------------------------------
 * Capability to a Page of memory:
 */
struct PageCap {
  enum Captype type;      // PageCap
  unsigned*    page;      // Pointer to the page
  unsigned     unused[2];
};

static inline unsigned* isPageCap(struct Cap* cap) {
  return (cap->type==PageCap) ? ((struct PageCap*)cap)->page : 0;
}

static inline struct PageCap* pageCap(struct Cap* cap, unsigned* page) {
  struct PageCap* pcap = (struct PageCap*)cap;
  printf("cap at %x for page %x\n", pcap, page);
  pcap->type = PageCap;
  pcap->page = page;
  return pcap;
}

/*-------------------------------------------------------------------------
 * Capability to a PageTable
 */
struct PageTableCap {
  enum Captype type;      // PageTableCap
  struct Ptab* ptab;      // Pointer to the page table
  unsigned     unused[2];
};

static inline struct Ptab* isPageTableCap(struct Cap* cap) {
  return (cap->type==PageTableCap) ? ((struct PageTableCap*)cap)->ptab : 0;
}

static inline
struct PageTableCap* pageTableCap(struct Cap* cap, struct Ptab* ptab) {
  struct PageTableCap* ptcap = (struct PageTableCap*)cap;
  printf("cap at %x for pagetable %x\n", ptcap, ptab);
  ptcap->type = PageTableCap;
  ptcap->ptab = ptab;
  return ptcap;
}

#endif
/*-----------------------------------------------------------------------*/

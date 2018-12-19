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
 * alloc.c:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "winio.h"		// For printf (debugging output)
#include "util.h"
#include "memory.h"
#include "alloc.h"
#include "caps.h"

/*-------------------------------------------------------------------------
 * Untyped memory areas:
 */
#define MAX_UNTYPED 32
static struct UntypedCap untyped[MAX_UNTYPED];
static unsigned numUntyped = 0;

void showUntyped() {
  printf("Available untyped(s) [%d]\n", numUntyped);
  struct UntypedCap* cap = untyped;
  for (int i=0; i<numUntyped; i++) {
    printf("  %02x: ", i);
    showCap((struct Cap*)cap++);
  }
}

static void insertUntyped(void* memory, unsigned bits) {
  printf("insert untyped %x (%d, %zd)\n", memory, bits, 1<<bits);
  struct UntypedCap* ucap = (struct UntypedCap*)untyped;
  for (int i=0; i<numUntyped; i++) {
    if (bits>ucap->bits) {  // The new page is bigger
      do {                  // shift up old elements
        void* nextmemory  = ucap->memory;
        ucap->memory      = memory;
        memory            = nextmemory;
        unsigned nextbits = ucap->bits;
        ucap->bits        = bits;
        bits              = nextbits;
        ++ucap;
      } while (++i<numUntyped);
      break;
    }
    ++ucap;
  }
  if (numUntyped<MAX_UNTYPED) { // extend the table, if possible
    untypedCap((struct Cap*)ucap, memory, bits);
    numUntyped++;
  }
}

static void addUntyped(unsigned lo, unsigned hi) {
  printf("Add untyped capabilities for [0x%08x-0x%08x]:\n", lo, hi);
  unsigned next = firstPageAfter(lo);
  if (next >= lo) {
    do {
      int      bits = PAGESIZE;
      unsigned mask = (1<<bits)-1;
      if (next+mask > hi) {
        break;
      }
      unsigned newmask;
      while ((next & (newmask=2*mask+1))==0 && next+newmask<=hi) {
        mask = newmask;
        if (++bits>=WORDSIZE) {
          break;
        }
      }
      unsigned start = next;
      insertUntyped(fromPhys(void*, start), bits);
      next += mask;
    } while (next++<hi);
  }
}

/*-------------------------------------------------------------------------
 * Donate the ith untyped capability, moving it to the specified capability
 * slot and removing it from the initial list of untyped caps.
 */
void donateUntyped(struct Cap* cap, unsigned i) {
  if (i<numUntyped && cap && isNullCap(cap)) {
    struct Cap* src = (struct Cap*)(untyped+i);
    moveCap(src, cap, 0);
    numUntyped--;
    for (int j=i; j<numUntyped; j++) {
      moveCap(src+1, src, 0);
      src++;
    }
  }
}

/*-------------------------------------------------------------------------
 * Interval management:
 */
#define MAX_INTERVALS 32
static unsigned numIntervals = 0;
static unsigned lo[MAX_INTERVALS];
static unsigned hi[MAX_INTERVALS];

static void showIntervals() {
  printf("{\n");
  for (unsigned i=0; i<numIntervals; i++) {
    printf("  [%x-%x] size=%zd\n", lo[i], hi[i], 1+hi[i]-lo[i]);
  }
  printf("}\n");
}

// Add a new interval to the current collection of separated intervals.
static void insertInterval(unsigned ilo, unsigned ihi) {
  printf("Inserting [%x-%x], ", ilo, ihi);
  ilo = firstPageAfter(ilo);
  ihi = endPageBefore(ihi);
  printf("rounded to [%x-%x]", ilo, ihi);

  if (ilo>=ihi) {  // region empty or too small to include
    return;
  }

  // Invariant: (writing n for numIntervals)
  //  - array segments [0..n), [0,m), [m,n) are all separated
  //  - all of intervals in (m,n) are separated from (ilo, ihi)

  unsigned m=numIntervals;
  while (m>0) {
    m--;
    // No action needed if interval m is separate from (ilo, ihi)
    if (!((ihi<lo[m] && (ihi+1)<lo[m]) || 
          (hi[m]<ilo && hi[m]<(ilo-1)))) {
      // expand interval (ilo, ihi) to include (lo[m], hi[m]):
      if (lo[m]<ilo) {
        ilo = lo[m];
      }
      if (hi[m]>ihi) {
        ihi = hi[m];
      }
      // remove last interval from end of array:
      if (--numIntervals>m) {
        lo[m] = lo[numIntervals];
        hi[m] = hi[numIntervals];
      }
    }
  }

  // Add (ilo, ihi) to the end of the array:
  if (numIntervals<MAX_INTERVALS) {
    lo[numIntervals] = ilo;
    hi[numIntervals] = ihi;
    numIntervals++;
  }
}

// Remove a reserved region from the current collection of intervals:
static void reserveInterval(unsigned rlo, unsigned rhi) {
  printf("Reserving [%x-%x], ", rlo, rhi);
  rlo = pageStart(rlo);  // reserve all pages enclosing (rlo,rhi)
  rhi = pageEnd(rhi);
  printf("rounded to [%x-%x]\n", rlo, rhi);

  if (rlo>=rhi) {        // empty region; cannot reserve
    return;
  }

  unsigned m=numIntervals;
  while (m>0) {
    m--;
    if (lo[m]<=rhi && rlo<=hi[m]) { 
      if (lo[m]<rlo) {
        if (rhi<hi[m]) {
          // Two intervals remain: (lo[m],rlo-1), (rhi+1,hi[m])
          lo[numIntervals] = rhi+1;  // Save second interval
          hi[numIntervals] = hi[m];  // at position numIntervals
          hi[m] = rlo-1;  // Save first interval in position m
          numIntervals++; // Increment total number of intervals
          // With the assumption that the stored list of intervals
          // are separated, we can conclude that no other interval
          // can be impacted by (rhi, rlo), and hence we can break
          // out of the loop early.  This also serves as a way to
          // document the fact that a single call to reserve() will
          // not increase the total number of intervals by more than
          // one.
          break;
        } else /* rhi>=hi[m] */ {
          // Truncate this interval to: (lo[m], rlo-1)
          hi[m] = rlo-1;
        }
     } else /* rlo <= lo[m] */ {
        if (rhi<hi[m]) {
          // Truncate this interval to: (rhi+1, hi[m])
          lo[m] = rhi+1;
        } else /* hi[m] <= rhi */ {
          // Remove this interval completely:
          if (--numIntervals>m) {
            lo[m] = lo[numIntervals];
            hi[m] = hi[numIntervals];
          }
        }
      }
    }
  }
}

static void sortIntervals() {
  for (int m=1; m<numIntervals; m++) {
    int      i = m-1;
    unsigned l = lo[i];
    for (int j=m; j<numIntervals; j++) {
      if (lo[j]<l) {
        l = lo[j];
        i = j;
      }
    }
    if (i>=m) {
      int tl = lo[m-1]; lo[m-1] = lo[i]; lo[i] = tl;
      int th = hi[m-1]; hi[m-1] = hi[i]; hi[i] = th;
    }
  }
}

/*-------------------------------------------------------------------------
 * Memory management: simple functionality for allocating pages of
 * memory for use in constructing page tables, etc.
 */
void initMemory(unsigned* hdrs, unsigned* mmap) {
  assert((1<<PAGESIZE) == KB(4),
         "pages are 4KB");

  assert(sizeof(struct Pdir) == KB(4),
         "page directories are 4KB long");
  assert(sizeof(struct Ptab) == KB(4),
         "page tables are 4KB long");
  assert(sizeof(struct Cspace) == KB(4),
         "capability spaces are 4KB long");
  
  assert(sizeof(struct Cap)*CSPACESIZE == KB(4),
         "capability spaces are 4KB long (second variant)");
  assert(sizeof(struct Cap) == sizeof(struct ConsoleCap),
         "console capabilities are the same size as other capabilities");
  assert(sizeof(struct Cap) == sizeof(struct WindowCap),
         "window capabilities are the same size as other capabilities");
  assert(sizeof(struct Cap) == sizeof(struct CspaceCap),
         "cspace capabilities are the same size as other capabilities");
  assert(sizeof(struct Cap) == sizeof(struct UntypedCap),
         "untyped capabilities are the same size as other capabilities");
  assert(sizeof(struct Cap) == sizeof(struct PageCap),
         "page capabilities are the same size as other capabilities");
  assert(sizeof(struct Cap) == sizeof(struct PageTableCap),
         "page table capabilities are the same size as other capabilities");

  // Add intervals for memory described in the memory map:
  printf("Memory map:\n");
  for (int i=0; i<mmap[0]; i++) {
    printf(" mmap[%d]: [%x-%x]\n",
           i, mmap[2*i+1], mmap[2*i+2]);
    insertInterval(mmap[2*i+1], mmap[2*i+2]);
    printf("\n");
  }

  // Reserve the first 1MB of memory and the kernel space:
  reserveInterval(0, MB(1)-1);
  reserveInterval(KERNEL_SPACE, ~0);

  // Reserve memory referenced in the headers:
  printf("Headers:\n");
  for (int i=0; i<hdrs[0]; i++) {
    printf(" header[%d]: [%x-%x], entry %x\n",
           i, hdrs[3*i+1], hdrs[3*i+2], hdrs[3*i+3]);
    reserveInterval(hdrs[3*i+1], hdrs[3*i+2]);
  }

  // Sort the intervals by increasing physical address:
  // (There is no real need to do this, but it makes the
  // debugging output easier to read!)
  sortIntervals();
  showIntervals();

  // Add Untyped memory areas corresponding to each interval:
  for (int i=0; i<numIntervals; i++) {
    addUntyped(lo[i], hi[i]);
  }
  showUntyped();

  assert(numUntyped>0, "there are no untyped memory objects");
}

/*-------------------------------------------------------------------------
 * Allocate a page of memory, drawing from the smallest available untyped
 * memory areas first.  Intended for use during kernel initialization only,
 * although it would still work later on if the untyped capabilities have
 * not all been donated to user-level processes/cspaces.
 */
unsigned* allocPage() {
  extern unsigned kernelInitialized;
  if (kernelInitialized) {
      fatal("calling allocPage() is not permitted after initialization");
  }
  for (; numUntyped>0; numUntyped--) {
    unsigned* page = (unsigned*)alloc(untyped+numUntyped-1, 12);
    if (page) {
      printf("allocated a page at %x\n", page);
      return page;
    }
  }
  fatal("Could not allocate a page");
  return 0; // not reached
}

/*-------------------------------------------------------------------------
 * Make a copy of a page of memory at a given physical address, returning
 * the physical address of the copy.
 */
unsigned copyPage(unsigned phys) {
  printf("copy page at physical address %x\n", phys);
  unsigned* src = fromPhys(unsigned*, phys);
  unsigned* dst = allocPage();
  phys          = toPhys(dst);
  assert(maskTo((unsigned)src, 12) == 0, "source is not 4K aligned");
  assert(maskTo((unsigned)dst, 12) == 0, "destination is not 4K aligned");
  for (int i=bytesToWords(1<<PAGESIZE); i>0; i--) {
    *dst++ = *src++;
  }
  return phys;
}

/*-----------------------------------------------------------------------*/

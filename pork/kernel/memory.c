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
 * Page-level kernel memory allocator:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "mimguser.h" 

#define DEBUG(cmd)   /*cmd*/

unsigned     numFreePages = 0;
static void* freePageList = 0;

/*-------------------------------------------------------------------------
 * Determine how many pages (out of a given maximum available) we should
 * take for kernel memory.
 */
static unsigned kernelPages(unsigned upper) {
  return upper / 8;
  //return min(10, upper);
}

/*-------------------------------------------------------------------------
 * Add a memory descriptor to the KIP.
 */
enum MemType {
  Virtual  = 0x201, Shared      = 0x004, Conventional = 0x001,
  Reserved = 0x002, BootModules = 0x02e, BootInfo     = 0x01e
};

extern unsigned MemoryInfo;
extern struct { unsigned hi; unsigned lo; } MemDesc[];

static void addMemDesc(unsigned lo, unsigned hi, enum MemType type) {
  unsigned n = mask(MemoryInfo, 16);
  if (lo<=hi) {
    abortIf(n>=MAX_MEMDESC, "Not enough memory descriptors");
    DEBUG(printf("Memory Descriptor %d: %x--%x\n", n, lo, hi);)
    MemDesc[n].lo = align(lo, 10) | type;
    MemDesc[n].hi = align(hi, 10);
    MemoryInfo    = align(MemoryInfo,16) | (n+1);
    DEBUG(printf("Memory Descriptor %d: %x--%x (adjusted)\n",
                    n, MemDesc[n].lo, MemDesc[n].hi);)
  }
}

/*-------------------------------------------------------------------------
 * Add pages to the free list.  The lo parameter gives the physical address
 * of the first available byte, and will be rounded up, if necessary to the
 * next page boundary.  The pages parameter specifies the number of available
 * pages, starting at the lo address.  (So, if lo is not already page aligned
 * then we will only be able to allocate pages-1 pages.)
 */
static void initPages(unsigned lo, unsigned pages) {
  unsigned hi = lo + (pages << PAGESIZE) - 1;
  unsigned pg = 1 << PAGESIZE;
  addMemDesc(lo, hi, Reserved); // Kernel Memory!
  DEBUG(printf("initPages(%x,%x)\n", lo, hi);)
  for (lo=align(lo+pg-1, PAGESIZE); (lo+pg-1)<=hi; lo+=pg) {
    DEBUG(printf("lo = %x, pg=%x\n", lo, pg);)
    freePage(fromPhys(void*, lo));
  }
}

/*-------------------------------------------------------------------------
 * Use boot data to initialize memory descriptors and memory allocator.
 */
struct MemRange { unsigned lo; unsigned hi; };
struct Header   { unsigned lo; unsigned hi; unsigned entry; };

void initMemory() {
  struct BootData* bd   = (struct BootData*)KERNEL_SPACE;
  unsigned numrngs      = *(bd->mmap);
  struct MemRange* rngs = (struct MemRange*)(bd->mmap+1);
  unsigned numhdrs      = *(bd->headers);
  struct Header* hdrs   = (struct Header*)(bd->headers+1);
  unsigned i, j;

  // Process headers for special tasks: -----------------------------------
  // headers: 0:bootdata  1:kernel    2:sigma0    3:roottask
  abortIf(numhdrs<3, "No sigma0");
  Sigma0Server.ip = hdrs[2].entry;
  Sigma0Server.lo = hdrs[2].lo;
  Sigma0Server.hi = hdrs[2].hi;

  if (numhdrs>=4) {
    RootServer.ip = hdrs[3].entry;
    RootServer.lo = hdrs[3].lo;
    RootServer.hi = hdrs[3].hi;
  } else {
    RootServer.ip = 0;
  }

  // Add Memory Descriptors to KIP: ---------------------------------------
  addMemDesc(0, KERNEL_SPACE-1, Virtual); // Virtual address space
  addMemDesc(0, 0xffffffff, Shared);      // Full 32 bit address space
  addMemDesc(0xa0000, 0xfffff, Shared);   // Video RAM, BIOS ROMs, etc...
  for (i=0; i<numrngs; i++) {             // Conventional memory regions
    addMemDesc(rngs[i].lo, rngs[i].hi, Conventional);
    DEBUG(printf(" rngs[%d]: [%x-%x]\n", i, rngs[i].lo, rngs[i].hi);)
  }
  for (j=0; j<numhdrs; j++) {             // Reserved boot modules
    addMemDesc(hdrs[j].lo, hdrs[j].hi, BootModules);
    DEBUG(printf(" hdrs[%d]: [%x-%x], entry %x\n",
                 j, hdrs[j].lo, hdrs[j].hi, hdrs[j].entry);)
  }

  // ----------------------------------------------------------------------
  // Find out which region in the memory map has most free space, taking
  // account of memory areas listed in headers and limited to the portion
  // of physical memory that is accessible within the PHYSMAP.
  unsigned maxsize = 0;        // size of biggest range
  unsigned maxidx  = numrngs;  // index of biggest range
  for (i=0; i<numrngs; i++) {
    // Find lo and hi addresses in this range, truncated to PHYSMAP
    unsigned lo = rngs[i].lo;
    unsigned hi = min(rngs[i].hi, PHYSMAP-1);
    if (lo < hi && ((1<<PAGESIZE)-1 < hi)) {
      // Round lo and hi to page start and end addresses
      lo = (((lo - 1) >> PAGESIZE) + 1) << PAGESIZE;
      hi = align(hi + 1, PAGESIZE) - 1;
      unsigned size = 1 + hi - lo;
      DEBUG(printf("  Page boundaries: [%x-%x] (size %x)\n", lo, hi, size);)
      // Scan headers to look for overlapping regions
      for (j=0; j<numhdrs; j++) {
        unsigned hlo = align(max(lo, hdrs[j].lo), PAGESIZE);
        unsigned hhi = ~align(~min(hi, hdrs[j].hi), PAGESIZE);
        if (hlo < hhi) {
          // It's possible to overestimate the amount of used space in
          // this region if two header ranges intersect the same physical
          // page.  However, this (1) isn't likely to happen very often;
          // and (2) isn't a serious problem.
          DEBUG(printf("  Size = %x\n", 1 + hhi - hlo);)
          size -= 1 + hhi -hlo;
        }
      }
      if (maxsize < size) {
        maxsize = size;
        maxidx  = i;
      }
    }
  }

  // ----------------------------------------------------------------------
  // Allocate kernel memory pages from the region with most free space
  if (maxidx < numrngs) {               // (assuming there is one ...)
    unsigned lo    = rngs[maxidx].lo;
    unsigned hi    = min(rngs[maxidx].hi, PHYSMAP-1);
    unsigned pages = kernelPages((1 + hi - lo) >> PAGESIZE);
    for (;;) {
      unsigned earidx = numhdrs;        // Find earliest allocated region
      unsigned earlo  = ~0;
      unsigned earhi  = 0;
      for (j=0; j<numhdrs; j++) {
        unsigned hlo = align(max(lo, hdrs[j].lo), PAGESIZE);
        unsigned hhi = ~align(~min(hi, hdrs[j].hi), PAGESIZE);
        if (hlo<hhi && hlo<earlo) {
          earidx = j;
          earlo  = hlo;
          earhi  = hhi;
        }
      }
      if (earidx==numhdrs) {            // No intersecting region found.
        initPages(lo, min((1 + hi - lo) >> PAGESIZE, pages));
        break;
      }
      if (earlo>lo) {
        unsigned maxp = min((earlo-lo) >> PAGESIZE, pages);
        initPages(lo, maxp);
        if ((pages -= maxp) == 0) {     // Are we done?
          break;
        }
      }
      lo = earhi+1;
    }
  }
  ASSERT(freePageList!=0, "Unable to allocate kernel memory");
  DEBUG(printf("numFreePages = %d\n", numFreePages);)
}

/*-------------------------------------------------------------------------
 * Allocate a single zeroed page of kernel memory from the free list.
 */
void* allocPage1() {
  ASSERT(freePageList!=0, "page allocate fails");
  void* result = freePageList;
  unsigned  i  = (1<<10);
  unsigned* p  = (unsigned*)result;
  numFreePages--;
  for (freePageList = *((void**)result); i>0; i--) {
    *p++ = 0;  // zero all elements; 
  }
  ASSERT(mask((unsigned)result, PAGESIZE)==0, "allocating misaligned page");
  ASSERT(((unsigned)p-(unsigned)result)==(1<<PAGESIZE), "zero page error");
  DEBUG(printf("Allocated page: %x, freePageList: %x\n",result,freePageList);)
  return result;
}

/*-------------------------------------------------------------------------
 * Return a page of kernel memory to the free list.
 */
void freePage(void* page) {
  ASSERT(mask((unsigned)page, PAGESIZE)==0, "free on misaligned page");
  DEBUG(printf("freePage(%x)\n", page);)
  *((void**)page) = freePageList;
  freePageList    = page;
  numFreePages++;
}

/*-------------------------------------------------------------------------
 * Test for available pages.  This whole approach will need a rethink if
 * we have genuinely concurrent accesses (e.g., SMP) that make demands on
 * the same free list ... a reservation system instead perhaps?
 */
bool availPages(unsigned n) {
  return numFreePages>=n;
}

/*-------------------------------------------------------------------------
 * Run a consistency check on the allocator's free list.
 */
void checkMem() {
  unsigned count = 0;
  void*    fr    = freePageList;
  printf("Checking free list ... ");
  while (fr) {
    count++;
    fr = *((void**)fr); 
  }
  printf("intact with %d elements (%d expected)\n",count, numFreePages);
}

/*-----------------------------------------------------------------------*/

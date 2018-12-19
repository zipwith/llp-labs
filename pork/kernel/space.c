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
 * Address Space Data Structures and Operations:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#include "pork.h"
#include "memory.h"
#include "space.h"

#define DEBUG(cmd)	/*cmd*/

struct Space {                  // Structure known only in this module
  unsigned        pdir;         // Physical address of page directory
  struct Mapping* mem;		// Memory map
  Fpage           kipArea;      // Location of kernel interface page
  Fpage           utcbArea;     // Location of UCTBs
  unsigned        count;        // Count of threads in this space
  unsigned        active;       // Count of active threads in this space
  unsigned        loaded;       // 1 => already loaded in cr3
};

/*-------------------------------------------------------------------------
 * Our implementation uses two kinds of objects, one to represent
 * complete address spaces (struct Space) and one to represent
 * individual memory mappings (struct Mapping).  These are both small
 * objects (less than 32 bytes or 8 words each).  We use ObjectPage
 * structures, each of which takes a 4K page of kernel memory, to
 * support dynamic allocation (and deallocation) of these Objects.
 * Each ObjectPage includes, along with header information, an array
 * of Object structures.  When all of the slots in one ObjectPage have
 * been taken, the system allocates another new page and begins to
 * allocate from that instead.  ObjectPages that have been allocated
 * but not filled are chained together in a doubly linked list of
 * "partials", which the Object allocator can use to satisfy requests.
 * Each ObjectPage also includes a free list that chains together any
 * Object structures in that page whose storage is no longer required.
 * If all of the Objects in a given ObjectPage are released, then
 * the whole page will be deallocated.  The array of ObjectPages
 * is initialized lazily as each slot in the array is used for the
 * first time.  This avoids the cost of initializing a complete page
 * in one operation (which, among other things, would not be very
 * cache friendly), and instead spreads the cost out over a sequence
 * of allocations.
 */
#define FULL 127		// Number of Objects per ObjectPage

struct Object {                 // A single object structure
  struct Object* next;
  unsigned pad[7];
};

struct ObjectPage {
  struct ObjectPage* prev;	// Doubly linked list of ObjectPages
  struct ObjectPage* next;
  unsigned           count;	// Number of active Objects in this page
  struct Object*     free;	// ptr to first Object in the free list
  struct Object*     last;	// addr of last Object that we allocated
  unsigned           pad[3];
  struct Object      blocks[FULL];
};

static struct ObjectPage* partials = 0;

/*-------------------------------------------------------------------------
 * Allocate a single Object, either from the list of partially filled
 * pages or, if necessary, by allocating a new page of kernel memory.
 */
static struct Object* allocObject1() {
  if (partials) {		// There are partially filled pages
    struct Object* obj = partials->free;
    if (obj) {			// Try to allocate from free list
      partials->free = obj->next;
    } else {			// Initialize next node in this page
      obj = ++partials->last;
    }
    if (++partials->count==FULL) {// If page is full, remove it from partials
      struct ObjectPage* objp = partials;
      if ((partials = objp->next)) {
        partials->prev = 0;
      }
    }
    return obj;
  } else {			// Need to allocate a new page
    partials = (struct ObjectPage*)allocPage1();
    partials->prev  = 0;
    partials->next  = 0;
    partials->free  = 0;
    partials->count = 1;
    return partials->last = partials->blocks;
  }
}

/*-------------------------------------------------------------------------
 * Deallocate a single Object, returning the storage to the free list in
 * the underlying Object, or deallocating the underlying page altogether
 * if this was the last Object in the page.
 */
static void freeObject(struct Object* obj) {
  struct ObjectPage* objp = (struct ObjectPage*)align((unsigned)obj,12);
  unsigned newcount       = objp->count - 1;
  if (newcount>0) {
    objp->count = newcount;
    if (newcount==FULL-1) {	// Is a full page is becoming partial?
      objp->prev = 0;
      objp->next = partials;
      if (partials) {
        partials->prev = objp;
      }
      partials = objp;
    }
    obj->next  = objp->free;	// Add obj to the free list for objp
    objp->free = obj;
  } else { 			// obj was the last active Object in objp
    struct ObjectPage* prev = objp->prev;
    struct ObjectPage* next = objp->next;
    if (prev) {
      prev->next = next;
    } else {
      partials   = next;
    }
    if (next) {
      next->prev = prev;
    }
    freePage(objp);
  }
}

/*-------------------------------------------------------------------------
 * Mapping database structures:
 *
 * We use Mapping structures for two distinct purposes:
 * 
 * 1) To provide a mapping database that describes the current mappings
 * between address spaces.  The mapping database is a tree of Mappings.
 * If an address space c receives a mapping from p, then c will be
 * included as a child of the parent p.  The mapping database tree is
 * actually represented as a doubly-linked list of Mappings using the
 * next and prev links.  The first node in the list is the root of the
 * tree, with the other nodes following in the order of a depth first
 * traversal.  Each node includes a level, which indicates its depth
 * from the root: the root has level 0, children of the root have
 * level 1, and so on.
 * 
 * 2) To describe the virtual addresses that are mapped in each address
 * space.  Conceptually, we can describe the mappings in an address
 * space by a set of disjoint intervals in virtual memory, but we will
 * actually represent these sets using binary search trees so that
 * they can be searched more efficiently.  (At some point, we should
 * switch to a balanced binary tree structure---such as red-black or AVL
 * trees---stealing a few bits from the level field to store whatever
 * balance information is needed.)  These trees are represented using
 * the left and right links, and we store a pointer to the root of each
 * tree in the corresponding address space.
 *-----------------------------------------------------------------------*/

struct Mapping {
  struct Space*   space;	// Which address space is this in?
  struct Mapping* next;
  struct Mapping* prev;
  unsigned        level;
  Fpage           vfp;		// Virtual fpage
  unsigned        phys;		// Physical address
  struct Mapping* left;
  struct Mapping* right;
};

/*-------------------------------------------------------------------------
 * Look for a mapping within the given fpage of a particular space.
 * Return a NULL pointer if no mapping can be found.
 */
static struct Mapping* findMapping(struct Space* space, Fpage vfp) {
  struct Mapping* m     = space->mem;
  struct Mapping* cand  = 0;
  unsigned        fpend = fpageEnd(vfp);
  while (m) {
    if (fpend < fpageStart(m->vfp)) {
      m = m->left;
    } else {
      cand = m;
      m    = m->right;
    }
  }
  return (cand && fpageStart(vfp)<fpageEnd(cand->vfp)) ? cand : 0;
}

/*-------------------------------------------------------------------------
 * Update a memory map by inserting a new mapping.
 * Add a new mapping for the fpage vfp into the specified space.  We
 * assume that vfp does not overlap with any existing mapping in the
 * space, and that vfp has non-zero permissions (or else this mapping
 * would not be useful).
 */
static struct Mapping* addMapping1(struct Space* space, Fpage vfp) {
  unsigned         base = fpageStart(vfp);
  struct Mapping** pm   = &space->mem;
  struct Mapping*  m;
  while ((m=*pm)) {
    pm = (base<fpageStart(m->vfp)) ? (&m->left) : (&m->right);
  }
  *pm      = m = (struct Mapping*)allocObject1();
  m->space = space;
  m->vfp   = vfp;
  m->left  = m->right = 0;
  return m;
}

/*-------------------------------------------------------------------------
 * Remove a mapping from the space in which it was allocated.  We assume
 * that the mapping was previously added to the space using addMapping1.
 */
static void removeMapping(struct Mapping* n) {
  struct Space*    space = n->space;
  unsigned         base  = fpageStart(n->vfp);
  struct Mapping** pn    = &space->mem;
  struct Mapping*  m;
  while ((m=*pn)!=n) {
    pn = (base<fpageStart(m->vfp)) ? (&m->left) : (&m->right);
  }
  // Now pn holds a pointer to n, which is the node that we want to delete.
  if (n->left==0) {          // left child of n is empty
    *pn = n->right;
  } else if (n->right==0) {  // right child of n is empty
    *pn = n->left;
  } else {                   // neither child is empty
    struct Mapping** pm = &n->right;
    while ((m=*pm)->left) {  // find leftmost node on right
      pm = &m->left;
    }
    *pm      = m->right;     // unlink m from right of tree
    *pn      = m;            // and use it to replace n
    m->left  = n->left;
    m->right = m->right;
  }
}

/*-------------------------------------------------------------------------
 * Page directories and page tables:
 *-----------------------------------------------------------------------*/

struct Pdir { unsigned pde[1024]; };
struct Ptab { unsigned pte[1024]; };

unsigned*           utcbptr;
static struct Ptab* utcbPtab;

/*-------------------------------------------------------------------------
 * Set the page directory control register to a specific value.
 */
static inline void setPdir(unsigned pdir) {
  asm("  movl  %0, %%cr3\n" : : "r"(pdir));
}

/*-------------------------------------------------------------------------
 * Return a pointer to the page table for the ith entry of the specified
 * pdir, or NULL if it is not present (0x1) or is a super page (0x80).
 */
static inline struct Ptab* getPagetab(struct Pdir* pdir, unsigned i) {
  return ((pdir->pde[i]&0x81)==0x1)
          ? fromPhys(struct Ptab*, align(pdir->pde[i], PAGESIZE)) : 0;
}

/*-------------------------------------------------------------------------
 * Return a pointer to the page for the ith entry of the specified ptab,
 * or NULL if it is not present (0x1).
 */
static inline void* getPage(struct Ptab* ptab, unsigned i) {
  return (ptab->pte[i]&1)
          ? fromPhys(void*, align(ptab->pte[i], PAGESIZE)) : 0;
}

/*-------------------------------------------------------------------------
 * Allocate a page directory for a new address space.  The user portion of
 * the virtual address space is initially empty, except for a kip mapping,
 * and the first 512MB of physical memory is mapped into the kernel space.
 */
static struct Pdir* allocPdir2(unsigned kipAddr) {
  struct Pdir* pdir = (struct Pdir*)allocPage1();
  unsigned     i    = 0;

  // Zero out user portion of the address space
  while (i<(KERNEL_SPACE>>SUPERSIZE)) {
    pdir->pde[i++] = 0;
  }

  // Create a mapping for the first PHYSMAP bytes of physical memory
  while (i<((KERNEL_SPACE+PHYSMAP)>>SUPERSIZE)) {
    pdir->pde[i] = ((i<<SUPERSIZE) - KERNEL_SPACE) | PERMS_KERNELSPACE;
    i++;
  }

  // Remaining portion of address space is also unmapped
  while (i<1024) {
    pdir->pde[i++] = 0;
  }

  // Add a 4K mapping for the UTCBPTR
  pdir->pde[UTCBPTR>>SUPERSIZE] = toPhys(utcbPtab) | PERMS_USER_RW;

  // Add a 4K mapping for the kip
  if (kipAddr < KERNEL_SPACE) {
    struct Ptab* ptab = (struct Ptab*)allocPage1();
    pdir->pde[kipAddr>>SUPERSIZE]
                      = toPhys(ptab) | PERMS_USER_RW;
    ptab->pte[mask(kipAddr>>PAGESIZE, 10)]
                      = toPhys(Kip)  | PERMS_USER_RO;
  }
  return pdir;
}

/*-------------------------------------------------------------------------
 * Free page directory storage once the last thread in an address space
 * has been deleted.  We assume that all user space mappings have already
 * been removed (and hence that all page tables for user space mappings
 * have already been deallocated).
 */
static void freePdir(struct Pdir* pdir, Fpage utcbArea) {
  // Free pages allocated to utcbs:
  unsigned p = fpageStart(utcbArea) >> PAGESIZE;
  unsigned e = fpageEnd(utcbArea)   >> PAGESIZE;
  while (p<=e) {
    struct Ptab* ptab = getPagetab(pdir, p>>10);
    if (ptab) {
      do {
        void* pg = getPage(ptab, mask(p,10));
        if (pg) {
          freePage(pg);
        }
      } while (++p<e && mask(p,10)!=0);
    } else if ((p>>10) < (e>>10)) {
        p += (1<<10);
    } else {
      break;
    }
  }

  // Free pages used to map utcbs and the kip:
  for (p=0; p<(KERNEL_SPACE>>SUPERSIZE); p++) {
    // TODO: we could optimize this ... we only need to scan the
    // page directory slots for the utcbArea and kipArea ...
    struct Ptab* ptab = getPagetab(pdir, p);
    if (ptab) {
      freePage(ptab);
    }
  }

  // Free page used for the top-level page directory:
  freePage(pdir);
}

/*-------------------------------------------------------------------------
 * Allocate a kernel mapped page to contain a utcb at the specified address
 * in the given page directory.
 */
static void* allocUtcbPage2(struct Pdir* pdir, unsigned utcbAddr) {
  unsigned i        = mask(utcbAddr>>SUPERSIZE, 10);
  struct Ptab* ptab = getPagetab(pdir, i);
  if (!ptab) {                            // No page table at this addr
    ptab         = (struct Ptab*)allocPage1();
    pdir->pde[i] = align(toPhys(ptab), PAGESIZE) | PERMS_USER_RW;
  }
  void* page = getPage(ptab, i=mask(utcbAddr>>PAGESIZE, 10));
  if (!page) {                            // No page at this address
    page         = (void*)allocPage1();
    ptab->pte[i] = align(toPhys(page), PAGESIZE) | PERMS_USER_RW;
  }
  return (void*)(page + mask(utcbAddr, PAGESIZE));
}

/*-------------------------------------------------------------------------
 * Update a page directory by mapping a given Fpage of virtual addresses
 * at the specified physical address.  We assume that this mapping does
 * not overlap any existing mapping.
 */
static void mapFpage1(struct Space* space, Fpage vfp, unsigned phys) {
  struct Pdir* pdir = fromPhys(struct Pdir*, space->pdir);
  unsigned     base = fpageStart(vfp);
  unsigned     size = fpageSize(vfp);
  unsigned     i    = base >> SUPERSIZE;
  phys              = align(phys, size)
                    | ((vfp & W) ? PERMS_USER_RW : PERMS_USER_RO);
  if (size>=SUPERSIZE) {       // Allocate fpage using 4MB super pages
    phys |= PERMS_SUPERPAGE;
    for (unsigned j = i+(1<<(size-SUPERSIZE)); i<j; i++) {
      pdir->pde[i] = phys;
      phys        += (1<<SUPERSIZE);
    }
  } else if (size>=PAGESIZE) { // Allocate fpage using 4KB pages
    struct Ptab* ptab = getPagetab(pdir, i);
    if (!ptab) {
      ptab         = (struct Ptab*)allocPage1();
      pdir->pde[i] = toPhys(ptab) | PERMS_USER_RW; // TODO: check this perm
    }
    i = mask(base>>PAGESIZE, 10);
    for (unsigned j = i+(1<<(size-PAGESIZE)); i<j; i++) {
      ptab->pte[i] = phys;
      phys        += (1<<PAGESIZE);
    }
  }
  space->loaded = 0; // Force page directory register (cr3) reload
}

/*-------------------------------------------------------------------------
 * Update a page directory by unmapping a given Fpage of virtual addresses.
 * We assume that the same fpage was previously mapped using mapFpage.
 */
static void unmapFpage(struct Space* space, Fpage vfp) {
  struct Pdir* pdir = fromPhys(struct Pdir*, space->pdir);
  unsigned     base = fpageStart(vfp);
  unsigned     size = fpageSize(vfp);
  unsigned     i    = base >> SUPERSIZE;
  if (size>=SUPERSIZE) {        /* Fpage allocated using 4MB super pages */
    for (unsigned j = i+(1<<(size-SUPERSIZE)); i<j; i++) {
      pdir->pde[i] = 0;
    }
  } else if (size>=PAGESIZE) {  /* Fpage allocated fpage using 4KB pages */
    struct Ptab* ptab = getPagetab(pdir, i);
    if (ptab) {
      if (findMapping(space, fpage(i<<SUPERSIZE, SUPERSIZE))
         || (space->kipArea>>SUPERSIZE == i)
	 || (space->utcbArea>>SUPERSIZE == i)) {
        // There are other mappings within this superpage (either user
	// mappings or kip/utcb mappings), so we just clear page table
	// entries here and don't delete the storage...
        i = mask(base>>PAGESIZE, 10);
        for (unsigned j = i+(1<<(size-PAGESIZE)); i<j; i++) {
          ptab->pte[i] = 0;
        }
      } else {
         // There are no remaining mappings within this superpage, so we
         // can free up the storage that was used for the page table.
         freePage(ptab);
         pdir->pde[i] = 0;
      }
    }
  }
  space->loaded = 0; // Force page directory register (cr3) reload
}

/*-------------------------------------------------------------------------
 * Operations on address spaces:
 *-----------------------------------------------------------------------*/

struct Space* sigma0Space;
struct Space* rootSpace;
static struct Space* currentSpace = 0;

unsigned fpsize[64], fpmask[64]; // Size and mask arrays for fpages

/*-------------------------------------------------------------------------
 * Initialize 
 */
void initSpaces() {
  // Basic consistency checks:
  ASSERT(sizeof(struct Object)     == 32,   "Object size error");
  ASSERT(sizeof(struct ObjectPage) == (1<<PAGESIZE), "ObjectPage size error");
  ASSERT(sizeof(struct Space)  <= sizeof(struct Object), "Space size error");
  ASSERT(sizeof(struct Mapping)<= sizeof(struct Object), "Mapping size error");
  ASSERT(mask((unsigned)Kip,PAGESIZE) == 0, "KIP alignment error");
  ASSERT((KipEnd-Kip) <= (1<<KIPAREASIZE),  "KIP size error");
  ASSERT(KIPAREASIZE <= PAGESIZE,           "KIP area size error");
  ASSERT(UTCBSIZE <= PAGESIZE,              "UTCB area size error");

  // Initialize fpage mask and size arrays.
  unsigned i;
  for (i=0; i<64; i++) {
    fpsize[i] = fpmask[i] = 0;
  }
  unsigned k = 0xfff;
  for (i=12; i<=32; i++) {
    fpsize[i] = i;
    fpmask[i] = k;
    k         = (k<<1)|1;
  }
  fpsize[1] = 32;
  fpmask[1] = ~0;

  // Initialization:
  abortIf(!availPages(5), "Unable to allocate initial address space");
  utcbptr      = (unsigned*)allocPage1();
  utcbPtab     = (struct Ptab*)allocPage1();
  utcbPtab->pte[mask(UTCBPTR>>PAGESIZE, 10)]
               = toPhys(utcbptr) | PERMS_USER_RO;
  sigma0Space  = allocSpace1();
  rootSpace    = allocSpace1();
  // Initialize mapping database:  TODO: this needs to be refined!
  struct Mapping* m = addMapping1(sigma0Space, completeFpage()|R|W|X);
  m->level = 1;
  m->next  = m->prev  = 0;
}

/*-------------------------------------------------------------------------
 * Determine whether a given address space is privileged.
 */
bool privileged(struct Space* space) {
  return (space==sigma0Space || space==rootSpace);
}

/*-------------------------------------------------------------------------
 * Allocate a new, (uninitialized) address space.
 */
struct Space* allocSpace1() {
  struct Space* space = (struct Space*)allocObject1();
  space->pdir         = 0;
  space->mem          = 0;
  space->kipArea      = 0;
  space->utcbArea     = 0;
  space->count        = 0;
  space->active       = 0;
  space->loaded       = 0;
  return space;
}

/*-------------------------------------------------------------------------
 * Signal that a new (inactive) thread is being added to this space.
 * We assume that this function is called exactly once for each new
 * thread that is created in the given address space, and that there
 * is at most one corresponding call to exitSpace() in each case.
 */
void enterSpace(struct Space* space) {
  space->count++;   // increment reference count;
}

/*-------------------------------------------------------------------------
 * Configure kip and utcb areas for a given space, assuming that the input
 * parameters are valid (not nilpage, non-overlapping, and in user space).
 */
void configureSpace(struct Space* space, Fpage kipArea, Fpage utcbArea) {
  ASSERT(!activeSpace(space), "configuring active space");
  space->kipArea  = kipArea;
  space->utcbArea = utcbArea;
}

/*-------------------------------------------------------------------------
 * Indicate whether a given address space has been configured or not.
 */
bool configuredSpace(struct Space* space) {
  return !isNilpage(space->utcbArea);
}

/*-------------------------------------------------------------------------
 * Return the kernel interface page start address in the given space:
 */
unsigned kipStart(struct Space* space) {
  return fpageStart(space->kipArea);
}

/*-------------------------------------------------------------------------
 * Determine whether a suggested utcbAddr is valid in a given area.
 */
bool validUtcb(struct Space* space, unsigned utcbAddr) {
  ASSERT(configuredSpace(space), "validating utcb in unconfigured space");
  return mask(utcbAddr, UTCBALIGN) == 0
      && fpageStart(space->utcbArea) <= utcbAddr
      && utcbAddr+(1<<UTCBSIZE)-1 <= fpageEnd(space->utcbArea);
}

/*-------------------------------------------------------------------------
 * Allocate a utcb structure at the specified address in the given space.
 * if this is this is the first thread to be activated, then we will also
 * construct and initialize a page directory structure for the space.
 * This will ensure that the space has valid mappings for both the kip
 * and utcb structures in the space so that the kernel is able to read
 * and/or write to those pages without triggering any page faults.
 * We assume that the address space has been initialized with a valid
 * utcbArea area and that the specified utcb address has been validated.
 */
void* allocUtcb4(struct Space* space, unsigned utcbAddr) {
  ASSERT(configuredSpace(space), "activating unconfigured space");
  ASSERT(validUtcb(space, utcbAddr), "activating with invalid address");
  struct Pdir* pdir;
  if (0==space->active++) {
    pdir        = allocPdir2(fpageStart(space->kipArea));
    space->pdir = toPhys(pdir);
  } else {
    pdir        = fromPhys(struct Pdir*, space->pdir);
  }
  space->loaded = 0;
  return allocUtcbPage2(pdir, utcbAddr);
}

/*-------------------------------------------------------------------------
 * Indicate whether a given address space is active or not (i.e., whether
 * it contains active threads/a valid page directory).
 */
bool activeSpace(struct Space* space) {
  return (space->pdir!=0);
}

/*-------------------------------------------------------------------------
 * Refresh the current address space, if necessary, after possible changes
 * to its page table structures.
 */
void refreshSpace() {
  if (!currentSpace->loaded) {   // Same thread, reload may be required
    setPdir(currentSpace->pdir);
    currentSpace->loaded = 1;
  }
}

/*-------------------------------------------------------------------------
 * Switch to a specified address space.  This is a nop if we are already
 * running in the specified space (hence saving the overhead of reloading
 * the page directory register), but we can force a reload (for example,
 * if the page table structure has been changed) by setting the loaded
 * flag of the space to 0 prior to calling switchSpace().
 */
void switchSpace(struct Space* space) {
  if (space->pdir) {               // No switch for kernel/inactive threads
    if (currentSpace!=space) {
      currentSpace = space;
      setPdir(currentSpace->pdir);
      currentSpace->loaded = 1;
    } else {
      refreshSpace();
    }
  }
}

/*-------------------------------------------------------------------------
 * Map a single page physical into sigma0's address space.
 */
unsigned sigma0map(unsigned addr) {  // TODO: quick hack; refine?
  if (availPages(1)) {
    addr = align(addr, PAGESIZE);
    mapFpage1(sigma0Space, fpage(addr, PAGESIZE)|R|W|X, addr);
    return 1;
  }
  return 0;
}

/*-------------------------------------------------------------------------
 * Flush a mapping, and all its descendants, from the mapping database.
 */
static void flush(struct Mapping* m) {
  struct Mapping* p = m->prev;
  unsigned        l = m->level;
  do {
    struct Mapping* next = m->next;
DEBUG(printf("flush %x [prev=%x, next=%x, level=%d]\n", m, p, next, m->level);)
    removeMapping(m);
DEBUG(printf("unmapping %x from %x\n", m->vfp, m->space);)
    unmapFpage(m->space, m->vfp);
DEBUG(printf("freeing %x\n", m);)
    freeObject((struct Object*)m);
    m = next;
DEBUG(printf("moving on to next (%x)...\n", m);)
  } while (m && (m->level > l));
DEBUG(printf("loop finishes at p=%x, m=%x\n", p, m);)
  if (p) p->next = m; // test for null p is unlikely to be necessary
  if (m) m->prev = p;
}

/*-------------------------------------------------------------------------
 * Create a mapping between address spaces.
 */
void map2(struct Space* sendspace, Fpage sendfp, unsigned sendbase,
          struct Space* recvspace, Fpage recvfp) {
DEBUG(printf("mapping %x,%x in space %x to %x in %x\n", sendfp, sendbase, sendspace, recvfp, recvspace);)
  // A map with the same send and recv space is a NOP:
  if (sendspace==recvspace) {
DEBUG(printf("same space, nop\n");)
    return;
  }

  // Ensure that neither page is nil:
  unsigned sendsize = fpageSize(sendfp);
  unsigned recvsize = fpageSize(recvfp);
  if (sendsize==0 || recvsize==0) {
DEBUG(printf("nil page, no map\n");)
    return;
  }

  // Adjust send and recv fpages to have the same size (save in recvsize)
  if (sendsize>recvsize) {
    sendfp   = trimFpage(sendfp, sendbase, recvfp);
  } else if (sendsize<recvsize) {
    recvfp   = trimFpage(recvfp, sendbase, sendfp);
    recvsize = sendsize;
  }
DEBUG(printf("adjusted mapping: %x,%x in space %x to %x in %x\n", sendfp, sendbase, sendspace, recvfp, recvspace);)

  // Check that send page is mapped in send space:
  struct Mapping* s = findMapping(sendspace, sendfp);
  if (s==0 || (sendsize=fpageSize(s->vfp))<recvsize) {
DEBUG(printf("send page is not mapped in send space\n");)
    return;
  }
  // sendfp and recvfp are fpages of size recvsize
  // s points to a Mapping of size sendsize that includes sendfp

  // Calculate permissions for receiving fpage:
  unsigned perms = (R|W|X) & sendfp & (s->vfp);
  if (perms==0) {
    return;
  }
  recvfp = (recvfp & ~(R|W|X)) | perms;
DEBUG(printf("receive fpage is %x\n");)

  // Check that recv page doesn't intersect kernel, kip, or utcb area
  unsigned recvstart = fpageStart(recvfp);
  unsigned recvend   = fpageEnd(recvfp);
  if (recvend>=KERNEL_SPACE
     || (recvstart<=fpageStart(recvspace->utcbArea) &&
         recvend>=fpageEnd(recvspace->utcbArea))
     || (recvstart<=fpageStart(recvspace->kipArea) &&
         recvend>=fpageEnd(recvspace->kipArea))) {
DEBUG(printf("oops, receive fpage intersects kernel, kip, or utcb area\n");)
    return;
  }

  // Look for pages that are already mapped in the recv fpage
  struct Mapping* t = findMapping(recvspace, recvfp);
  if (t) {
DEBUG(printf("clearing already mapped pages in recv space\n");)
    // We can't flush t until we're commited to doing this map; so there
    // better not be any more checks after this ...
    unsigned tsize = fpageSize(t->vfp);
DEBUG(printf("tsize=%d, recvsize=%d, sendsize=%d\n", tsize, recvsize, sendsize);)
    if (tsize<recvsize) {
      // t is too small to cover recvfp, so (1) there might be other
      // fpages mapped in recvfp, which means that we need a loop to
      // make sure we find them all; and (2) neither t nor any of the
      // other nodes that we might find in this range are big enough
      // to have s as a descendant.
      do {
        flush(t);
      } while ((t=findMapping(recvspace, recvfp)));
    } else {
DEBUG(printf("case 2\n");)
      // t is at least as big as recvfp, so we know that there won't be any
      // other mappings that intersect recvfp.  However, we need to check
      // that s is not a descendant of t.  Fortunately, we know that, if s
      // is a descendant of t, then (a) t would be at least as big as s;
      // and (b) s would be at a higher level (i.e., further from the
      // root) than t.  As a result, we know that we can skip a potentially
      // expensive test if either one of these conditions does not hold.
      if (tsize>=sendsize && s->level>t->level) {
        // Loop through descendants of t to look for s
        unsigned        l = t->level;
        struct Mapping* n = t->next;
DEBUG(printf("test holds, level l=%d\n", l);)
        for (; n->level<l; n=n->next) {
DEBUG(printf("n->level=%d\n", n->level);)
          if (n==s) {
DEBUG(printf("***** IGNORING MAPPING \n");)
              return;
          }
        }
DEBUG(printf("finished loop\n");)
      }
DEBUG(printf("about to flush\n");)
DEBUG(extern void showMapping(unsigned ind, struct Mapping* m);)
DEBUG(extern void showMappingDB();)
DEBUG(showMapping(3, t);)
DEBUG(showMappingDB();)
      flush(t);
DEBUG(printf("done flush\n");)
    }
DEBUG(printf("done clear\n");)
  }
DEBUG(printf("all checks cleared, proceeding with mapping\n");)

  // We've validated all parameters and cleared out any memory mapped
  // into the recv fpage.  No more excuses; time to add the mapping!
  t        = addMapping1(recvspace, recvfp);
  t->level = 1 + s->level;
  t->prev  = s;
  if ((t->next=s->next)) {
    t->next->prev = t;
  }
  s->next  = t;
  mapFpage1(recvspace, recvfp,
            t->phys = (s->phys) + fpageStart(sendfp) - fpageStart(s->vfp));
DEBUG(printf("mapping completed\n");)
}

/*-------------------------------------------------------------------------
 * Signal that a thread is being removed from an address space.  We assume
 * that there is a corresponding earlier matching enterSpace() call for
 * each use of this function.
 */
void exitSpace(struct Space* space, void* utcb) {
DEBUG(extern void showSpace(struct Space* space);)
DEBUG(extern void showMappingDB();)
DEBUG(extern void showMapping(unsigned ind, struct Mapping* m);)
DEBUG(printf("exitSpace %x, active=%d\n", space, space->active);)
DEBUG(printf("BEFORE:\n");)
DEBUG(showSpace(space);)
DEBUG(showMappingDB();)
  // If this was the last active thread in the address space, then we
  // reclaim any mapping and page directory storage space that was in use.
  if (utcb && --space->active==0) {  // Last active thread in space?
    // Flush all of the memory mapping nodes for this space:
    // TODO: this could probably be done better if we avoided flush()
DEBUG(printf("exitSpace: flushing space\n");)
    for (struct Mapping* m; (m=space->mem); ) {
DEBUG(printf("--------------------------\ntree: %x\n", m);)
DEBUG(showMapping(2, m);)
      for (struct Mapping* l; (l=m->left)!=0; m=l) {
        m->left  = l->right;                 // rotate to avoid recursion
        l->right = m;
DEBUG(printf("after rotation:\n");)
DEBUG(showMapping(4, l);)
      }
      flush(space->mem = m);
    }
    // Free the page directory for this space:
DEBUG(printf("exitSpace: free page directory\n");)
    freePdir(fromPhys(struct Pdir*, space->pdir), space->utcbArea);
  }

  // If this was the last thread in the address space, then we can also
  // reclaim storage for the complete space structure.  An exception is
  // made for privileged spaces, which are never deleted, even if they
  // become empty.  This ensures that no other space can be allocated
  // later at the same address and accidentally obtain privileges that
  // it was not intended to have ...
  if (--space->count==0 && !privileged(space)) {
DEBUG(printf("exitSpace: free space object\n");)
    freeObject((struct Object*)space);
  }
DEBUG(else { printf("AFTER:\n"); showSpace(space); })
DEBUG(showMappingDB();)
}

/*-------------------------------------------------------------------------
 * Output a description of a page directory.
 */
void showPdir(struct Pdir* pdir) { // TODO: Remove me; debugging code
  printf("  Page directory at %x\n", pdir);
  for (unsigned i=0; i<1024; i++) {
    if (pdir->pde[i]&1) {
      if (pdir->pde[i]&0x80) {
        printf("    %x: [%x-%x] => [%x-%x], superpage\n",
               i, (i<<SUPERSIZE), ((i+1)<<SUPERSIZE)-1,
               align(pdir->pde[i], SUPERSIZE),
               align(pdir->pde[i], SUPERSIZE) + 0x3fffff);
      } else {
        struct Ptab* ptab = fromPhys(struct Ptab*,
                                     align(pdir->pde[i], PAGESIZE));
        unsigned base = (i<<SUPERSIZE);
        printf("    [%x-%x] => page table at %x (physical %x):\n",
               base, base + (1<<SUPERSIZE)-1,
               ptab, align(pdir->pde[i], PAGESIZE));
        for (unsigned j=0; j<1024; j++) {
          if (ptab->pte[j] & 1) {
            printf("      %x: [%x-%x] => [%x-%x] page\n",
                   j, base+(j<<PAGESIZE), base + ((j+1)<<PAGESIZE) - 1,
                   align(ptab->pte[j], PAGESIZE),
                   align(ptab->pte[j], PAGESIZE) + 0xfff);
          }
        }
      }
    }
  }
}

/*-------------------------------------------------------------------------
 * Print a description of a mapping.
 */
void showMapping(unsigned ind, struct Mapping* m) {
  if (m) {
    showMapping(ind+1, m->left);
    for (unsigned i=0; i<ind; i++) {
      printf(" ");
    }
    printf("[%x-%x], phys=%x, level=%x, space=%x\n",
           fpageStart(m->vfp), fpageEnd(m->vfp), m->phys, m->level, m->space);
    showMapping(ind+1, m->right);
  }
}

/*-------------------------------------------------------------------------
 * Print a description of the current mapping database.
 */
void showMappingDB() {
  struct Mapping* m = sigma0Space->mem;
  printf("mapping database, root=%x\n");
  while (m) {
    for (unsigned i=0; i<m->level; i++) {
      printf("  ");
    }
    printf("%x[%x-%x], phys=%x, level=%x, space=%x, prev=%x, next=%x\n",
           m,
           fpageStart(m->vfp), fpageEnd(m->vfp), m->phys, m->level, m->space,
           m->prev, m->next);
    m = m->next;
  }
}

/*-------------------------------------------------------------------------
 * Print a description of a space.
 */
void showSpace(struct Space* space) {
  printf("address space %x\n", space);
  printf("  count %d, active %d, loaded %d\n",
         space->count, space->active, space->loaded);
  printf("  kipArea %x: [%x-%x]\n", space->kipArea,
         fpageStart(space->kipArea), fpageEnd(space->kipArea));
  printf("  utcbArea %x: [%x-%x]\n", space->utcbArea,
         fpageStart(space->utcbArea), fpageEnd(space->utcbArea));
  printf("  Mappings:\n");
  showMapping(3, space->mem);
  if (activeSpace(space)) {
    printf("  Page directory at physical address: %x\n", space->pdir);
    showPdir(fromPhys(struct Pdir*, space->pdir));
  } else {
    printf("  No mappings in %x: Space is not yet activated\n", space);
  }
}

/*-----------------------------------------------------------------------*/

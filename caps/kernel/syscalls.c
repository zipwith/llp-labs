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
 * syscalls.c: System call handlers
 * Mark P Jones + YOUR NAME HERE, Portland State University
 *-----------------------------------------------------------------------*/
#include "winio.h"
#include "util.h"
#include "context.h"
#include "proc.h"
#include "paging.h"
#include "memory.h"
#include "hardware.h"
#include "alloc.h"
#include "caps.h"

/*-------------------------------------------------------------------------
 * System call to dump current status data for the invoking thread in the
 * console window.
 */
void dump_imp() {
  struct Context* ctxt = &current->ctxt;
  showPdir(current->pdir);
  showCspace(current->cspace);
  showUntyped();
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * System call to map a page at a specific address using the kernel to
 * allocate the necessary storage for the page and, possibly, a page table.
 */
// Determine if there is already a mapping for a page at a given address.
static unsigned isMapped(struct Pdir* pdir, unsigned virt) {
  virt = alignTo(virt, PAGESIZE);
  if (virt<KERNEL_SPACE) {
    unsigned dir = maskTo(virt >> SUPERSIZE, 10);
    unsigned pde = pdir->pde[dir];
    if ((pde&1)==0) {                 // directory entry not in use
      return 0;
    } else if ((pde&0x80)==0) {       // not mapped to a superpage
      unsigned     idx  = maskTo(virt >> PAGESIZE, 10);
      struct Ptab* ptab = fromPhys(struct Ptab*, alignTo(pde, PAGESIZE));
      return (ptab->pte[idx] & 1);    // return 0 if already mapped
    }
  }
  return 1;
}

void kmapPage_imp() {
  struct Context* ctxt = &current->ctxt;
  unsigned        addr = ctxt->regs.esi;
  unsigned*       page;

  // NOTE: This system call breaks the rule that there should be
  // no dynamic memory allocation in the kernel after it has been
  // initialized.  DO NOT emulate the techniques here in your code!
  printf("warning: kmapPage() may call allocPage()\n");
  extern unsigned kernelInitialized;
  kernelInitialized = 0;  // HACK: do not replicate this in your code!

  if (!isMapped(current->pdir, addr) && (page=allocPage())) {
    //printf("Mapping to virtual address %x to page at %x\n", addr, page);
    mapPage(current->pdir, addr, toPhys(page));
    ctxt->regs.eax = 1;
  } else {
    printf("Could not map a page at %x\n", addr);
    ctxt->regs.eax = 0;
  }

  kernelInitialized = 1;
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * Console (kernel output window) system call.
 */
void kputc_imp() {
  struct Context*    ctxt = &current->ctxt;
  struct ConsoleCap* cap  = isConsoleCap(current->cspace->caps +
                                         cptr(ctxt->regs.ecx));
  if (cap) {
    setAttr(cap->attr);
    putchar(ctxt->regs.eax);
    setAttr(7);
    ctxt->regs.eax = (unsigned)current;
  } else {
    ctxt->regs.eax = 0;
  }
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * Window output system calls.
 */
struct WindowCap* getWindowCap() {
  return isWindowCap(current->cspace->caps + cptr(current->ctxt.regs.ecx));
}

void putchar_imp() {
  struct WindowCap* wcap = getWindowCap();
  if (wcap && (wcap->perms & CAN_putchar)) {
    wputchar(wcap->window, current->ctxt.regs.eax);
  }
  switchToUser(&current->ctxt);
}

void cls_imp() {
  struct WindowCap* wcap = getWindowCap();
  // TODO: complete implementation for this function:
  // do not allow the window to be cleared unless the
  // invoking process has specified a valid WindowCap
  // with CAN_cls permission.  The capability number
  // will be specified in the ecx register.
  switchToUser(&current->ctxt);
}

void setAttr_imp() {
  struct WindowCap* wcap = getWindowCap();
  // TODO: complete implementation for this function
  // do not allow the window to be cleared unless the
  // invoking process has specified a valid WindowCap
  // with CAN_setAttr permission.  The capability number
  // will be specified in the ecx register, while the
  // value in eax determines the new attribute.
  switchToUser(&current->ctxt);
}

/*-------------------------------------------------------------------------
 * Lookup a capability that is specified by an index and a cptr pair.
 */
static struct Cap* getCap(unsigned slot) {
  printf("getCap(%x), index=%x, cptr=%x\n", slot, index(slot), cptr(slot));
  struct Cspace* cspace = isCspaceCap(current->cspace->caps + index(slot));
  printf("cspace = %x, ret=%x\n", cspace,
         (cspace ? (cspace->caps + cptr(slot)) : 0));
  return cspace ? (cspace->caps + cptr(slot)) : 0;
}

void capmove_imp() {
  struct Context* ctxt = &current->ctxt;
  struct Cap*     src  = getCap(ctxt->regs.esi);
  struct Cap*     dst  = getCap(ctxt->regs.edi);
  unsigned        copy = ctxt->regs.eax;
  printf("Capmove: %x -> %x %s\n",
          ctxt->regs.esi,
          ctxt->regs.edi,
          (copy ? "copy" : "move"));
  printf("  src=%x, dst=%x\n", src, dst);
  if ((dst && src && isNullCap(dst) && !isNullCap(src)) &&
      (!copy || src->type!=UntypedCap)) { // Cannot copy an untyped cap
    printf("  Before:\n");
    showCspace(current->cspace);
    moveCap(src, dst, ctxt->regs.eax);
    printf("  After:\n");
    showCspace(current->cspace);
    ctxt->regs.eax = 1;
  } else {
    printf("  Invalid capmove\n");
    ctxt->regs.eax = 0;
  }
  switchToUser(ctxt);
}

void capclear_imp() {
  struct Context* ctxt = &current->ctxt;
  // TODO: complete implementation for this function:
  // Input argument (in esi) provides an index in the
  // invoking process' cpsace to a CspaceCap as well
  // as a cptr (in the least significant byte) to a
  // particular slot in that cspace.  If the referenced
  // capability can be found and is not a NullCap, then
  // it will be set to NullCap and the value 1 will be
  // returned in eax.  If the operation is not permitted
  // then the return code will be zero.
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * System calls for allocating objects from an untyped memory area.
 *
 * In each case:
 *   ecx = capability to untyped memory
 *   edi = destination slot for capability to new object
 *
 * These allocator calls are very similar (the only differences being in
 * the way that the new objects are initialized and the ability to allow
 * a variable (bits) size for untyped objects).  It might be nice to
 * combine them in a single system call that takes the object type as an
 * extra parameter ...
 */
struct UntypedCap* getUntypedCap() {
  return isUntypedCap(current->cspace->caps + cptr(current->ctxt.regs.ecx));
}

// allocUntyped also uses: eax = log size of new untyped to allocate
void allocUntyped_imp() {
  struct Context*    ctxt = &current->ctxt;
  struct UntypedCap* ucap = getUntypedCap();
  struct Cap*        cap  = getCap(ctxt->regs.edi);
  unsigned           bits = ctxt->regs.eax;
  void*              obj;
  printf("allocUntyped: bits %d from ucap=%x, slot=%x\n", bits, ucap, cap);
  if (ucap &&                     // valid untyped capability
      cap  && isNullCap(cap) &&   // empty destination slot
      validUntypedSize(bits) &&   // bit size in legal range
      (obj=alloc(ucap, bits))) {  // object allocation succeeds
    untypedCap(cap, obj, bits);
    ctxt->regs.eax = 1;
  } else {
    ctxt->regs.eax = 0;
  }
  switchToUser(ctxt);
}

void allocCspace_imp() {
  struct Context*    ctxt = &current->ctxt;
  struct UntypedCap* ucap = getUntypedCap();
  struct Cap*        cap  = getCap(ctxt->regs.edi);
  void*              obj;
  printf("allocCspace: from ucap=%x, slot=%x\n", ucap, cap);
  if (ucap &&                        // valid untyped capability
      cap  && isNullCap(cap) &&      // empty destination slot
      (obj=alloc(ucap, PAGESIZE))) { // object allocation succeeds
    cspaceCap(cap, (struct Cspace*)obj);
    ctxt->regs.eax = 1;
  } else {
    ctxt->regs.eax = 0;
  }
  switchToUser(ctxt);
}

void allocPage_imp() {
  struct Context*    ctxt = &current->ctxt;
  // TODO: Using the capability to untyped memory that is referenced
  // by ecx in the invoking process' cspace, allocate a new Page
  // object and place a capability to the new object in the slot of
  // the invoking process' cspace at the position specified by edi.
  // Return code in eax should be 1 for success, 0 for failure.
  switchToUser(ctxt);
}

void allocPageTable_imp() {
  struct Context*    ctxt = &current->ctxt;
  // TODO: Using the capability to untyped memory that is referenced
  // by ecx in the invoking process' cspace, allocate a new Page
  // object and place a capability to the new object in the slot of
  // the invoking process' cspace at the position specified by edi.
  // Return code in eax should be 1 for success, 0 for failure.
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * Address space management:
 */
void mapPage_imp() {
  struct Context* ctxt = &current->ctxt;
  // TODO: This syetm call takes two parameters:
  //   ecx should identify a capability in the invoking process'
  //       cspace to a Page object:
  //   eax should specify an address in the current process'
  //       address space where that page should be mapped.
  // For this operation to succeed: the address should be within
  // the user space region of memory; there should already be a
  // page table (not a super page) in the appropriate slot of the
  // page directory; but there should not be a page mapped at the
  // specified address.  You may choose either to ignore the least
  // significant bits of the address (the page offset) or, if you
  // prefer, to decline the request for a mapping if those bits
  // are not zero.  A return code of 1 (success) or 0 (error)
  // should be produced as normal.
  switchToUser(ctxt);
}

void mapPageTable_imp() {
  struct Context* ctxt = &current->ctxt;
  // TODO: This syetm call takes two parameters:
  //   ecx should identify a capability in the invoking process'
  //       cspace to a PageTable object:
  //   eax should specify an address in the current process'
  //       address space where that pagetable should be mapped.
  // For this operation to succeed: the address should be within
  // the user space region of memory and the corresponding entry
  // in the page directory should not be present.  You may choose
  // either to ignore the least significant 22 bits of the address
  // (the superpage offset) or, if you prefer, to decline the
  // request for a mapping if those bits are not zero.  A return
  // code of 1 (success) or 0 (error) should be produced as normal.
  switchToUser(ctxt);
}

/*-------------------------------------------------------------------------
 * Timer interrupt handler:
 */
static void tick() {
  static unsigned ticks = 0;
  static char*    spin = "-\\|/";  // characters for a rotatating spinner
  static unsigned pos  = 0;        // current position within spinner
  ticks++;
  if ((ticks&15)==0) {
    // Calculate the memory location where the character in the top
    // right corner of the video RAM is stored (skipping the first
    // 79 columns, each of which takes two bytes).
    char *vid = (char*)(KERNEL_SPACE + 0xb8000 + (2*79));
    // Update the spinner character:
    *vid = spin[(pos++)&3];
    // Perform a context switch:
    reschedule();
  }
}

void timerInterrupt() {
  maskAckIRQ(TIMERIRQ);           // Mask and acknowledge timer interrupt
  enableIRQ(TIMERIRQ);            // TODO: can this be optimized?
  tick();
  switchToUser(&current->ctxt);
}

/*-----------------------------------------------------------------------*/

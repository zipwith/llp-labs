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
 * Flexpage and Address Space Data Structures:
 * Mark P Jones, Portland State University
 *-----------------------------------------------------------------------*/
#ifndef SPACE_H
#define SPACE_H

/*-------------------------------------------------------------------------
 * The Flexpage datatype:
 *-----------------------------------------------------------------------*/
typedef unsigned Fpage;

static inline Fpage fpage(unsigned base, unsigned size) {
  return align(base, size) | (size<<4);
}

static inline Fpage completeFpage(void) { // [0::Bit 22 | 1::Bit 6 |0|r|w|x]
  return (1<<4);
}

extern unsigned fpsize[];
// initialized to 0 -> 0, 1 -> 32, 2 -> 0, ..., 11 -> 0,
// 12 -> 12, 13 -> 13, ..., 32 -> 32, 33 -> 0, ...
extern unsigned fpmask[];
// initialized to 0 -> 0, 1 -> ~0, 2 -> 0, ..., 11 -> 0, 
// 12 -> 0xfff, 13 -> 0x1fff, ..., 32 -> 0xffffffff, 33 -> 0, ...

static inline unsigned fpageMask(Fpage fp)  { return fpmask[(fp>>4)&0x3f]; } 
static inline unsigned fpageSize(Fpage fp)  { return fpsize[(fp>>4)&0x3f]; }
static inline bool     isComplete(Fpage fp) { return ~fpageMask(fp) == 0; }
static inline bool     isNilpage(Fpage fp)  { return fpageMask(fp) == 0;  }
static inline unsigned fpageStart(Fpage fp) { return fp & ~fpageMask(fp); }
static inline unsigned fpageEnd(Fpage fp)   { return fp | fpageMask(fp);  }

/* Trim a given fpage to a smaller size, using a base value to determine
 * which portion of the larger page should be used.
 */
static inline Fpage trimFpage(Fpage big, unsigned base, Fpage small) {
  unsigned bmask = fpageMask(big);
  unsigned smask = fpageMask(small);
  return (big & ((R|W|X)|~bmask))  // perms and upper bits from big
       | (base & bmask & ~smask)   // lower bits from base
       | (small & 0x3f0);          // size from small
}

/*-------------------------------------------------------------------------
 * Address Spaces:
 *-----------------------------------------------------------------------*/
struct Space;

extern struct Space* sigma0Space;
extern struct Space* rootSpace;

extern void          initSpaces(void);
extern bool          privileged(struct Space* space);
extern struct Space* allocSpace1(void);
extern void          enterSpace(struct Space* space);
extern void          configureSpace(struct Space* space,
                                    Fpage kipArea, Fpage utcbArea);
extern bool          configuredSpace(struct Space* space);
extern unsigned      kipStart(struct Space* space);
extern bool          validUtcb(struct Space* space, unsigned utcbAddr);
extern void*         allocUtcb4(struct Space* space, unsigned utcbAddr);
extern bool          activeSpace(struct Space* space);
extern void          switchSpace(struct Space* space);
extern void          refreshSpace(void);
extern unsigned      sigma0map(unsigned addr);
extern void          map2(struct Space* sendspace, Fpage sendfp,
                          unsigned sendbase,
                          struct Space* recvspace, Fpage recvfp);
extern void          exitSpace(struct Space* space, void* utcb);

#endif
/*-----------------------------------------------------------------------*/

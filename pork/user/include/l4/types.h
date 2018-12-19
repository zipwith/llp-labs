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
#ifndef L4_TYPES_H
#define L4_TYPES_H

/* Basic Definitions: ------------------------------------------------------*/

#if defined(__cplusplus)
#define EXTERNC(stuff) extern "C" { stuff; }
#else
#define EXTERNC(stuff) extern stuff;
#endif

#define L4_32BIT
#define L4_LITTLE_ENDIAN

typedef unsigned long      L4_Word_t;
typedef unsigned long long L4_Word64_t;
typedef unsigned long      L4_Word32_t;
typedef unsigned short     L4_Word16_t;
typedef unsigned char      L4_Word8_t;

typedef unsigned int       L4_Size_t;
typedef L4_Word_t          L4_Bool_t;

/* Fpages: -----------------------------------------------------------------*/

typedef struct {
    L4_Word_t raw;
} L4_Fpage_t;

#define L4_Readable        (0x04)
#define L4_Writable        (0x02)
#define L4_eXecutable      (0x01)
#define L4_FullyAccessible (0x07)
#define L4_ReadWriteOnly   (0x06)
#define L4_ReadeXecOnly    (0x05)
#define L4_NoAccess        (0x00)

#define L4_Nilpage              ((L4_Fpage_t){raw:0})
#define L4_CompleteAddressSpace ((L4_Fpage_t){raw:(1<<6)})

static inline L4_Fpage_t L4_FpageLog2(L4_Word_t baseAddress,
                                      int log2FpageSize) {
  return (L4_Fpage_t)
            {raw: (baseAddress & ~0x3ff)|((log2FpageSize & 0x3f)<<4)};
}

static inline L4_Fpage_t L4_Fpage(L4_Word_t baseAddress,
                                  int fpageSize) {
  /* TODO: do something to improve this horrible code! :-( */
  L4_Word_t log2 = 10;
  for (fpageSize>>=(log2+1); fpageSize>0; fpageSize>>=1) {
    log2++;
  }
  return L4_FpageLog2(baseAddress,log2);
}

static inline L4_Word_t L4_SizeLog2(L4_Fpage_t f) {
  return (f.raw>>4) & 0x3f;
}

static inline L4_Word_t L4_Size(L4_Fpage_t f) {
  L4_Word_t size = L4_SizeLog2(f);
  return (size<10) ? 0 : (1<<size);
}

static inline L4_Word_t L4_Address(L4_Fpage_t f) {
  L4_Word_t size = L4_SizeLog2(f);
  return f.raw & ~((1<<size)-1);
}

static inline L4_Bool_t L4_IsNilFpage(L4_Fpage_t f) {
  return L4_SizeLog2(f) < 10;
}

static inline L4_Word_t L4_Rights(L4_Fpage_t f) {
  return f.raw & L4_FullyAccessible;
}

static inline L4_Fpage_t L4_Set_Rights(L4_Fpage_t* f,
                                       L4_Word_t accessRights) {
  f->raw = (f->raw & ~L4_FullyAccessible)
         | (accessRights & L4_FullyAccessible);
  return *f;
}

static inline L4_Fpage_t L4_FpageAddRights(L4_Fpage_t f,
                                           L4_Word_t accessRights) {
  f.raw |= (accessRights & L4_FullyAccessible);
  return f;
}

static inline L4_Fpage_t L4_FpageAddRightsTo(L4_Fpage_t* f,
                                             L4_Word_t accessRights) {
  f->raw |= (accessRights & L4_FullyAccessible);
  return *f;
}

/* TODO: add FpageRemoveRights and FpageRemoveRightsFrom ... */

#if defined(__cplusplus)
static inline L4_Fpage_t operator+(const L4_Fpage_t& f,
                                   L4_Word_t accessRights) {
  return L4_FpageAddRights(f, accessRights);
}

static inline L4_Fpage_t operator+=(L4_Fpage_t& f,
                                    L4_Word_t accessRights) {
  return L4_FpageAddRightsTo(&f, accessRights);
}

/* TODO: add operator- and operator -= */

#endif /* __cplusplus */

/* Error codes: ------------------------------------------------------------*/

#define L4_ErrNoPrivilege        (1)
#define L4_ErrInvalidThread      (2)
#define L4_ErrInvalidSpace       (3)
#define L4_ErrInvalidScheduler   (4)
#define L4_ErrInvalidParam       (5)
#define L4_ErrUtcbArea           (6)
#define L4_ErrKipArea            (7)
#define L4_ErrNoMem              (8)
#define L4_ErrInvalidRedirector  (9)

#endif

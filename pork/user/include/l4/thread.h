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
#ifndef L4_THREAD_H
#define L4_THREAD_H
#include <l4/types.h>
#include <l4/utcb.h>

/* Thread Ids: -----------------------------------------------------------*/

typedef struct {
    L4_Word_t raw;
} L4_ThreadId_t;

#define L4_nilthread      ((L4_ThreadId_t){raw:0})
#define L4_anythread      ((L4_ThreadId_t){raw:~0U})
#define L4_anylocalthread ((L4_ThreadId_t){raw:(~0U)<<14})

static inline L4_ThreadId_t L4_GlobalId(L4_Word_t threadNo, L4_Word_t version) {
  L4_ThreadId_t tid;
  tid.raw = (threadNo<<14) | version;
  return tid;
}

static inline L4_Word_t L4_Version(L4_ThreadId_t t) {
  return (t.raw & 0x3fff);
}

static inline L4_Word_t L4_ThreadNo(L4_ThreadId_t t) {
  return (t.raw >> 14);
}

static inline L4_Bool_t L4_IsNilThread(L4_ThreadId_t t) {
    return t.raw == 0;
}

static inline L4_Bool_t L4_IsLocalId(L4_ThreadId_t t) {
    return (t.raw & 0x3f) == 0;
}

static inline L4_Bool_t L4_IsGlobalId (L4_ThreadId_t t) {
    return (t.raw & 0x3f) != 0;
}

static inline L4_ThreadId_t L4_LocalIdOf(L4_ThreadId_t t) {
  // TODO: incomplete implementation
  return t;
}

static inline L4_ThreadId_t L4_GlobalIdOf(L4_ThreadId_t t) {
  if (!L4_IsLocalId(t)) {
    // TODO: incomplete implementation
  }
  return t;
}

static inline L4_Bool_t L4_IsThreadEqual(const L4_ThreadId_t l,
                                         const L4_ThreadId_t r) {
  return l.raw == r.raw;
}

static inline L4_Bool_t L4_IsThreadNotEqual(const L4_ThreadId_t l,
                                            const L4_ThreadId_t r) {
  return l.raw != r.raw;
}

/* Thread Ids: (C++ Bindings) --------------------------------------------*/

#if defined(__cplusplus)

static inline L4_Bool_t operator==(const L4_ThreadId_t& l,
                                   const L4_ThreadId_t& r) {
  return l.raw == r.raw;
}

static inline L4_Bool_t operator!=(const L4_ThreadId_t& l,
                                   const L4_ThreadId_t& r) {
  return l.raw != r.raw;
}

static inline L4_ThreadId_t L4_LocalId(L4_ThreadId_t t) {
  return L4_LocalIdOf(t);
}

static inline L4_ThreadId_t L4_GlobalId(L4_ThreadId_t t) {
  return L4_GlobalIdOf(t);
}

#endif

/* Accessor methods: -----------------------------------------------------*/

static inline L4_ThreadId_t L4_MyGlobalId() {
  return ((L4_ThreadId_t*)L4_GetUtcb())[-16];
}

static inline L4_ThreadId_t L4_Myself() {
  return (L4_ThreadId_t)L4_MyGlobalId();
}

static inline L4_Word_t L4_ProcessorNo() {
  return ((L4_Word_t*)L4_GetUtcb())[-12];
}

static inline L4_ThreadId_t L4_Pager() {
  return ((L4_ThreadId_t*)L4_GetUtcb())[-10];
}

static inline void L4_Set_Pager(L4_ThreadId_t newPager) {
  ((L4_ThreadId_t*)L4_GetUtcb())[-10] = newPager;
}

static inline L4_ThreadId_t L4_ExceptionHandler() {
  return ((L4_ThreadId_t*)L4_GetUtcb())[-9];
}

static inline void L4_Set_ExceptionHandler(L4_ThreadId_t newHandler) {
  ((L4_ThreadId_t*)L4_GetUtcb())[-9] = newHandler;
}

static inline L4_Word_t L4_ErrorCode() {
  return ((L4_Word_t*)L4_GetUtcb())[-7];
}

/* System Calls: ---------------------------------------------------------*/

EXTERNC(L4_Word_t L4_ThreadControl(
			L4_ThreadId_t dest,
			L4_ThreadId_t spaceSpec,
			L4_ThreadId_t scheduler,
			L4_ThreadId_t pager,
			void* utcbLocation))

static inline L4_Word_t L4_AssociateInterrupt(
                        L4_ThreadId_t irqId,
                        L4_ThreadId_t handlerId) {
  return L4_ThreadControl(irqId, irqId, L4_nilthread, handlerId, (void*)(-1)); 
}

static inline L4_Word_t L4_DeassociateInterrupt( L4_ThreadId_t irqId) {
  return L4_AssociateInterrupt(irqId, irqId);
}

EXTERNC(L4_Word_t L4_Prim_ExchangeRegisters(
	L4_ThreadId_t  dest,
	L4_Word_t      control,
	L4_Word_t      sp,
	L4_Word_t      ip,
	L4_Word_t      flags,
	L4_Word_t      userDefinedHandle,
	L4_ThreadId_t  pager,
	L4_Word_t*     oldControl,
	L4_Word_t*     oldSp,
	L4_Word_t*     oldIp,
	L4_Word_t*     oldFlags,
	L4_Word_t*     oldUserDefinedHandle,
	L4_ThreadId_t* oldPager))

static inline L4_ThreadId_t L4_ExchangeRegisters(
        L4_ThreadId_t  dest,
        L4_Word_t      control,
	L4_Word_t      sp,
	L4_Word_t      ip,
	L4_Word_t      flags,
	L4_Word_t      userDefinedHandle,
	L4_ThreadId_t  pager,
	L4_Word_t*     oldControl,
	L4_Word_t*     oldSp,
	L4_Word_t*     oldIp,
	L4_Word_t*     oldFlags,
	L4_Word_t*     oldUserDefinedHandle,
	L4_ThreadId_t* oldPager) {
  return (L4_ThreadId_t){raw:L4_Prim_ExchangeRegisters(
        dest, control, sp, ip, flags, userDefinedHandle, pager,
	oldControl, oldSp, oldIp, oldFlags, oldUserDefinedHandle,
	oldPager)};
}

EXTERNC(void L4_Start_SpIpFlags(
             L4_ThreadId_t dest,
             L4_Word_t     sp,
             L4_Word_t     ip,
             L4_Word_t     flags))

#if defined(__cplusplus)

static inline void L4_Start(L4_ThreadId_t t,
                            L4_Word_t sp,
                            L4_Word_t ip) {
  L4_Start_SpIpFlags(t, sp, ip, 0);
}

static inline void L4_Start(L4_ThreadId_t t,
                            L4_Word_t sp,
                            L4_Word_t ip,
                            L4_Word_t flags) {
  L4_Start_SpIpFlags(t, sp, ip, flags);
}

#endif

#endif

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
#ifndef L4_KIP_H
#define L4_KIP_H
#include <l4/types.h>

/* Kernel Interface Page: ------------------------------------------------*/

typedef struct {
  L4_Word_t magic;
  L4_Word_t apiVersion;
  L4_Word_t apiFlags;
  L4_Word_t kernDescPtr;
  L4_Word_t pad1[17];
  L4_Word_t memoryInfo;
  L4_Word_t pad2[19];
  L4_Word_t virtRegInfo;
  L4_Word_t utcbInfo;
  L4_Word_t kipAreaInfo;
  L4_Word_t pad3[2];
  L4_Word_t bootInfo;
  L4_Word_t procDescPtr;
  L4_Word_t clockInfo;
  L4_Word_t threadInfo;
  L4_Word_t pageInfo;
  L4_Word_t processorInfo;
  L4_Word_t scSpaceControl;
  L4_Word_t scThreadControl;
  L4_Word_t scProcessorControl;
  L4_Word_t scMemoryControl;
  L4_Word_t scIpc;
  L4_Word_t scLipc;
  L4_Word_t scUnmap;
  L4_Word_t scExchangeRegisters;
  L4_Word_t pad4[1];
  L4_Word_t scThreadSwitch;
  L4_Word_t scSchedule;
  L4_Word_t pad5[1];
} L4_KernelInterfacePage_t;

EXTERNC(void* L4_KernelInterface(
			L4_Word_t* apiVersion,
			L4_Word_t* apiFlags,
			L4_Word_t* kernelId))

static inline void* L4_GetKernelInterface() {
  L4_Word_t apiVersion, apiFlags, kernelId;
  return L4_KernelInterface(&apiVersion, &apiFlags, &kernelId);
}

#if defined(__cplusplus)
static inline void* L4_KernelInterface() {
  return L4_GetKernelInterface();
}
#endif

static inline L4_Word_t L4_ApiVersion() {
  L4_Word_t apiVersion, apiFlags, kernelId;
  L4_KernelInterface(&apiVersion, &apiFlags, &kernelId);
  return apiVersion;
}

static inline L4_Word_t L4_ApiFlags() {
  L4_Word_t apiVersion, apiFlags, kernelId;
  L4_KernelInterface(&apiVersion, &apiFlags, &kernelId);
  return apiFlags;
}

static inline L4_Word_t L4_KernelId() {
  L4_Word_t apiVersion, apiFlags, kernelId;
  L4_KernelInterface(&apiVersion, &apiFlags, &kernelId);
  return kernelId;
}

/* Missing:
    L4_KernelGenDate
    L4_KernelVersion
    L4_KernelSupplier
 */

static inline L4_Word_t L4_NumProcessors(void* kip) {
  L4_Word_t processorInfo = ((L4_KernelInterfacePage_t*)kip)->processorInfo;
  return 1 + (processorInfo & 0xffff);
}

static inline L4_Word_t L4_NumMemoryDescriptors(void* kip) {
  L4_Word_t memoryInfo = ((L4_KernelInterfacePage_t*)kip)->memoryInfo;
  return memoryInfo & 0xffff;
}

static inline L4_Word_t L4_PageSizeMask(void* kip) {
  L4_Word_t pageInfo = ((L4_KernelInterfacePage_t*)kip)->pageInfo;
  return pageInfo & ~0x3ff;
}

static inline L4_Word_t L4_PageRights(void* kip) {
  L4_Word_t pageInfo = ((L4_KernelInterfacePage_t*)kip)->pageInfo;
  return pageInfo & 0x7;
}

static inline L4_Word_t L4_ThreadIdBits(void* kip) {
  L4_Word_t threadInfo = ((L4_KernelInterfacePage_t*)kip)->threadInfo;
  return threadInfo & 0xff;
}

static inline L4_Word_t L4_ThreadIdSystemBase(void* kip) {
  L4_Word_t threadInfo = ((L4_KernelInterfacePage_t*)kip)->threadInfo;
  return (threadInfo>>8) & 0xfff;
}

static inline L4_Word_t L4_ThreadIdUserBase(void* kip) {
  L4_Word_t threadInfo = ((L4_KernelInterfacePage_t*)kip)->threadInfo;
  return (threadInfo>>20) & 0xfff;
}

/* Missing:
    L4_ReadPrecision
    L4_SchedulePrecision
 */

static inline L4_Word_t L4_UtcbAreaSizeLog2(void* kip) {
  L4_Word_t utcbInfo = ((L4_KernelInterfacePage_t*)kip)->utcbInfo;
  return (utcbInfo>>16) & 0x3f;
}

static inline L4_Word_t L4_UtcbAlignmentLog2(void* kip) {
  L4_Word_t utcbInfo = ((L4_KernelInterfacePage_t*)kip)->utcbInfo;
  return (utcbInfo>>10) & 0x3f;
}

static inline L4_Word_t L4_UtcbSize(void* kip) {
  L4_Word_t utcbInfo = ((L4_KernelInterfacePage_t*)kip)->utcbInfo;
  L4_Word_t a        = (utcbInfo>>10) & 0x3f;
  return (1<<a) * (utcbInfo & 0x3ff);
}

static inline L4_Word_t L4_KipAreaSizeLog2(void* kip) {
  L4_Word_t kipAreaInfo = ((L4_KernelInterfacePage_t*)kip)->kipAreaInfo;
  return kipAreaInfo & 0x3f;
}

/* Missing:
    L4_BootInfo
    L4_KernelVersionString
    L4_Feature
 */

/* Memory Descriptors: ---------------------------------------------------*/

#define L4_UndefinedMemoryType            (0x0)
#define L4_ConventionalMemoryType         (0x1)
#define L4_ReservedMemoryType             (0x2)
#define L4_DedicatedMemoryType            (0x3)
#define L4_SharedMemoryType               (0x4)
#define L4_BootLoaderSpecificMemoryType   (0xe)
#define L4_ArchitectureSpecificMemoryType (0xf)

typedef struct {
  L4_Word_t raw[2];
} L4_MemoryDesc_t;

static inline L4_MemoryDesc_t* L4_MemoryDesc(void* kip, L4_Word_t num) {
  L4_Word_t memoryInfo = ((L4_KernelInterfacePage_t*)kip)->memoryInfo;
  return (num >= (memoryInfo & 0xffff))
            ? (L4_MemoryDesc_t*)0
            : (L4_MemoryDesc_t*)((L4_Word_t)kip + (memoryInfo>>16)) + num;
}

static inline L4_Bool_t L4_IsMemoryDescVirtual(L4_MemoryDesc_t* m) {
  return (m->raw[0]>>10) & 1;
}

static inline L4_Word_t L4_MemoryDescType(L4_MemoryDesc_t* m) {
  return (m->raw[0]) & 0xff;
}

static inline L4_Word_t L4_MemoryDescLow(L4_MemoryDesc_t* m) {
  return (m->raw[0]) & ~0x3ff;
}

static inline L4_Word_t L4_MemoryDescHigh(L4_MemoryDesc_t* m) {
  return (m->raw[1]) | 0x3ff;
}

#if defined(__cplusplus)

static inline L4_Bool_t L4_IsVirtual(L4_MemoryDesc_t* m) {
  return L4_IsMemoryDescVirtual(m);
}

static inline L4_Word_t L4_Type(L4_MemoryDesc_t* m) {
  return L4_MemoryDescType(m);
}

static inline L4_Word_t L4_Low(L4_MemoryDesc_t* m) {
  return L4_MemoryDescLow(m);
}

static inline L4_Word_t L4_High(L4_MemoryDesc_t* m) {
  return L4_MemoryDescHigh(m);
}

#endif

/* Processor Descriptors: ------------------------------------------------*/

typedef struct {
  L4_Word_t raw[4];
} L4_ProcessorDesc_t;

/* Missing:
    L4_ProcDesc
 */

#endif

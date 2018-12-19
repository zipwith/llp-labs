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
#include "simpleio.h"
#include "kip.h"
#include <l4/kip.h>

#define mask(e, a) ((e)&((1<<(a))-1))   // extract lower a bits of e
#define align(e,a) (((e)>>(a))<<(a))    // clear lower a bits of e

void showKIP() {
  L4_Word_t apiVersion;
  L4_Word_t apiFlags;
  L4_Word_t kernelId;
  struct KernelInterface* kip
    = (struct KernelInterface*)
       L4_KernelInterface(&apiVersion, &apiFlags, &kernelId);

  printf("----Displaying Kernel Interface Page mapped at 0x%x\n", kip);
  struct KernelDesc* kde
      = (struct KernelDesc*)(kip->kernDescPtr + (unsigned)kip);
  static char* months[]
   = {"---", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "xxx", "yyy", "zzz"};

  printf("%s\n", kde->features);
  printf("Kernel        : %c%c%c%c, ", kde->supplier[0], kde->supplier[1],
                                       kde->supplier[2], kde->supplier[3]);
  printf("id %d.%d, ",      (kde->id)>>24, mask((kde->id)>>16, 8));
  printf("version %d.%d, ", (kde->ver)>>24, mask((kde->ver)>>16, 8));
  printf("generated %s %d, %d\n", months[mask(kde->gendate>>5, 4)],
                                   mask(kde->gendate, 5),
                                   mask(kde->gendate>>9, 7) + 2000);

  printf("Magic Number  : %c%c%c%c\n", kip->magic[0], kip->magic[1],
                                       kip->magic[2], kip->magic[3]);
  printf("API           : version 0x%x, flags 0x%x (", apiVersion, apiFlags);
  switch (apiFlags & 0x3) {
    case 0x0 : printf("little endian"); break;
    case 0x1 : printf("big endian"); break;
    default  : printf("unknown endianness"); break;
  }
  printf(", ");
  switch (apiFlags & 0xc) {
    case 0x0 : printf("32 bit API"); break;
    case 0x4 : printf("64 bit API"); break;
    default  : printf("unknown bitwidth API"); break;
  }
  printf(")\n");

  struct MemDesc* mem
      = (struct MemDesc*)((kip->memoryInfo>>16) + (unsigned)kip);
  unsigned numMem = mask(kip->memoryInfo, 16);
  printf("Memory Descriptors:\n");
  for (unsigned i = 0; i<numMem; i++) {
    printf("  0x%x: [0x%x - 0x%x] 0x%x\n", i, align(mem[i].lo, 10),
                                      ~align(~mem[i].hi, 10),
                                      mask(mem[i].lo, 10));
  }
  
  struct ProcDesc* pds = (struct ProcDesc*)(kip->procDescPtr + (unsigned)kip);
  unsigned procs       = 1 + mask(kip->processorInfo, 16);
  printf("ProcessorInfo : %d processors, ProcDesc size=%d\n",
          procs, 1<<mask(kip->processorInfo>>28, 4));
  for (int i=0; i<procs; i++) {
    printf("  Processor %d : external %dMHz, frequency %dMHz\n",
           i, pds[i].externalFreq, pds[i].internalFreq);
  }
  printf("VirtRegInfo   : %d MRs\n", 1+(kip->virtRegInfo));
  printf("ThreadInfo    : user base %d, system base %d, thread bits %d\n",
         mask(kip->threadInfo>>20, 12), mask(kip->threadInfo>>8, 12),
         mask(kip->threadInfo, 8));
  printf("UtcbInfo      : s=%d, a=%d, multiplier=%d\n",
                 mask(kip->utcbInfo>>16, 6),
                 mask(kip->utcbInfo>>10, 6),
                 mask(kip->utcbInfo, 10));
  printf("KipAreaInfo   : size=%d\n", kip->kipAreaInfo);
  printf("PageInfo      : supported access rights %c%c%c, page sizes: ",
            ((kip->pageInfo & 4) ? 'r' : '-'),
            ((kip->pageInfo & 2) ? 'w' : '-'),
            ((kip->pageInfo & 1) ? 'x' : '-'));
  unsigned count = 10;
  unsigned mult  = 1;
  char*    suff  = "K";
  for (unsigned pbits = kip->pageInfo >> 10; pbits; pbits>>=1) {
    if (pbits&1) {
      printf(" %d%s", mult, suff);
    }
    switch (++count) {
      case 20 : suff = "M"; mult = 1; break;
      case 30 : suff = "G"; mult = 1; break;
      default : mult <<= 1;
    }
  }
  printf("\n");

  printf("\nPrivileged System Calls:\n");
  printf("SpaceControl      : 0x%x    ",(unsigned)kip + kip->scSpaceControl);
  printf("ThreadControl     : 0x%x\n", (unsigned)kip + kip->scThreadControl);
  printf("ProcessorControl  : 0x%x    ", (unsigned)kip + kip->scProcessorControl);
  printf("MemoryControl     : 0x%x\n", (unsigned)kip + kip->scMemoryControl);

  printf("\nSystem Calls:\n");
  printf("Ipc               : 0x%x    ", (unsigned)kip + kip->scIpc);
  printf("Lipc              : 0x%x\n", (unsigned)kip + kip->scLipc);
  printf("Unmap             : 0x%x    ", (unsigned)kip + kip->scUnmap);
  printf("ExchangeRegisters : 0x%x\n", (unsigned)kip + kip->scExchangeRegisters);
  printf("ThreadSwitch      : 0x%x    ", (unsigned)kip + kip->scThreadSwitch);
  printf("Schedule          : 0x%x\n", (unsigned)kip + kip->scSchedule);
  printf("-------------------------------------------------------\n");
}


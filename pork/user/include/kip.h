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

struct KernelInterface {
  char     magic[4];
  unsigned apiVersion;
  unsigned apiFlags;
  unsigned kernDescPtr;
  unsigned pad1[17];
  unsigned memoryInfo;
  unsigned pad2[19];
  unsigned virtRegInfo;
  unsigned utcbInfo;
  unsigned kipAreaInfo;
  unsigned pad3[2];
  unsigned bootInfo;
  unsigned procDescPtr;
  unsigned clockInfo;
  unsigned threadInfo;
  unsigned pageInfo;
  unsigned processorInfo;
  unsigned scSpaceControl;
  unsigned scThreadControl;
  unsigned scProcessorControl;
  unsigned scMemoryControl;
  unsigned scIpc;
  unsigned scLipc;
  unsigned scUnmap;
  unsigned scExchangeRegisters;
  unsigned pad4[1];
  unsigned scThreadSwitch;
  unsigned scSchedule;
  unsigned pad5[1];
};

struct KernelDesc {
  unsigned id;
  unsigned gendate;
  unsigned ver;
  char     supplier[4];
  char     features[0];
};

struct MemDesc {
  unsigned hi;
  unsigned lo;
};

struct ProcDesc {
  unsigned externalFreq;
  unsigned internalFreq;
};


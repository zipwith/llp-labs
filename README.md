This repository contains the source code for the Languages
and Low Level Programming (LLP) sample programs and lab
exercises.  These materials were developed for and used
in the courses on Languages and Low-Level Programming that
were taught at Portland State University in Spring 2015,
Spring 2016, and Fall 2017.  Further details about the LLP
course may be found at http://web.cecs.pdx.edu/~mpj/llp/.

## Acknowledgements:

The development of the materials in this repository was
supported in part by funding from the National Science
Foundation, Award No. CNS-1422979.

## Overview:

The `LABS` folder provides the source materials for the series
of seven LLP labs as they were presented to students.  Further
details are included in the `README.md` file for that folder.

The remaining folders in this repository contain the source
code for the demos and starting points for the full set of
labs.  For those who wish to use these materials
independently (i.e., without using the files in `LABS`), we
would suggest working through them in the following order:

* `vram` - A video RAM simulation, providing an introduction
  to low-level programming that runs in a conventional Linux
  desktop environment.

* `hello` - A simple bare metal demo that will boot and run,
  displaying a colorful "hello" greeting, on standard PC
  hardware, or in a virtual machine using a tool like QEMU
  or VirtualBox.

* `simpleio` - A library for simple I/O, including output to
  the screen (via video RAM) or a serial port, and input
  from the keyboard.

* `bootinfo` - A simple bare metal program that accesses and
  displays information that is passed in to it by the GRUB
  bootloader.

* `mimg` - Memory image boot tools, providing an extra layer
  of flexibility and control over Grub.  The mimgmake tool
  allows a developer to combine the contents of multiple
  executable or other files in a single image, including
  details of the addresses where the files should be loaded.
  The mimgload tool uses the resulting image files to ensure
  that memory is appropriately initialized at boot-time
  before handing control to the appropriate kernel code.

* `example-mimg` - A simple bare metal program that is
  desiged to be booted and executed using `mimgload`.  This
  program displays details from the boot data that are
  passed on to it by `mimgload`.

* `example-gdt` - A variant of `example-mimg` that includes
  code to initialize a global descriptor table (GDT) for
  controlling segmentation on the x86 platform.

* `example-idt` - An extension of `example-gdt` that
  includes code to initialize an interrupt descriptor table
  (IDT), and then uses interrupts to support switching
  between user and kernel (protected) mode programs.

* `switching` - An near copy of the `example-idt` demo that
   serves as the starting point for the context switching
   lab.

* `winio` - An expanded version of the `simpleio` library
  that implements support for using multiple (nonoverlapping)
  video RAM "window"s for character output.

* `paging` - An expanded version of `switching` that
  includes extra code and prompts for working through the
  paging and processes labs.

* `pork` - An implementation of a small, L4-like microkernel
  for the x86 platform that demonstrates techniques for memory
  and thread management, interprocess communication (IPC),
  etc..

* `caps` - A simple capability-based operating system that
  demonstrates key aspects of the capability abstraction in
  microkernels like seL4 but in a simpler setting that is
  suitable for a first introduction to the topic.  The files
  in this folder are used as the starting point for the
  LLP capabilities lab.


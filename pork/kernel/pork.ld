OUTPUT_FORMAT(elf32-i386)
ENTRY(physentry)

SECTIONS {

  physentry = entry - KERNEL_SPACE;
  . = KERNEL_LOAD + KERNEL_SPACE;

  .text ALIGN(0x1000) : AT(ADDR(.text) - KERNEL_SPACE) {
    _text_start = .; *(.text) *(.handlers) _text_end = .;
    *(.rodata*)
    *(.data)
    _start_bss = .; *(COMMON) *(.bss) _end_bss = .;
  }
}

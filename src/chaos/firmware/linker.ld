SECTIONS
{
  . = 0x100000;
  .text : { *(.text) }
  .rodata : AT(ADDR(.rodata))
  {
    . = ALIGN(8);
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(8);
  }
  .data : { *(.data) }
  .bss : { *(.bss) }
}

ENTRY(_start);

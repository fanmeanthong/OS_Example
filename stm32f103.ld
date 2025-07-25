/* STM32F103C8T6 - 64KB Flash, 20KB RAM */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);  /* Stack pointer reset value */

/* Symbols for startup */
_sidata = LOADADDR(.data);
_sdata  = .;
_edata  = .;
_sbss   = .;
_ebss   = .;

SECTIONS
{
  /* Vector table */
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  /* Code section */
  .text : 
  {
    *(.text*)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
  } > FLASH

  /* Initialized data section */
  .data : AT (LOADADDR(.text) + SIZEOF(.text))
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM

  /* Uninitialized data (zero-initialized in startup) */
  .bss (NOLOAD) :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  /* Optional: put task stack in separate section if needed */
  .task_stack (NOLOAD) :
  {
    *(.task_stack)
  } > RAM

  /* Stack top symbol (optional, already set by _estack) */
  . = ALIGN(4);
}

.altmacro
.section .text

.macro ISR_ERR vector
isr_\vector:
    pushal
    pushl 32(%esp)
    pushl $\vector
    call isr_dispatch
    addl $8, %esp
    popal
    addl $4, %esp
    iret
.endm

.macro ISR_NOERR vector
isr_\vector:
    pushal
    pushl $0
    pushl $\vector
    call isr_dispatch
    addl $8, %esp
    popal
    iret
.endm

.set i, 0
.rept 256
    .if (i == 8) || (i >= 10 && i <= 14) || (i == 17) || (i == 21)
        ISR_ERR %i
    .else
        ISR_NOERR %i
    .endif
    .set i, i + 1
.endr

.section .rodata

.macro ISR_ENTRY vector
    .long isr_\vector
.endm

.global isr_table
isr_table:
.set i, 0
.rept 256
    ISR_ENTRY %i
    .set i, i + 1
.endr

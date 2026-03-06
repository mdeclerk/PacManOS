.section .text
.global _start
.type _start, @function
_start:
    cli
    movl $stack_top, %esp
    push %ebx
    call init

hang:
    cli
    hlt
    jmp hang

.size _start, .-_start

.section .bss
.align 16
stack:
    .space 4096 * 4
stack_top:

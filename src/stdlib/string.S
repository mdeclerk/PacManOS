.section .text

/* --------------------------------------------------------------------------
 * void *memset(void *dst, int value, size_t size)
 *
 * Stack after push %edi (1 callee-save):
 *   8(%esp) = dst    
 *  12(%esp) = value    
 *  16(%esp) = size
 * Returns dst in %eax.
 * -------------------------------------------------------------------------- */
.global memset
.type memset, @function
memset:
    pushl   %edi

    movl    8(%esp), %edi       /* dst */
    movzbl  12(%esp), %eax      /* value – zero-extend low byte */
    movl    16(%esp), %ecx      /* size */

    cld

    /* Replicate the byte into all four bytes of %eax:
       eax = byte | (byte << 8) | (byte << 16) | (byte << 24)       */
    imull   $0x01010101, %eax, %eax

    /* Store dwords */
    movl    %ecx, %edx
    shrl    $2, %ecx            /* ecx = size / 4 */
    rep stosl

    /* Store remaining bytes */
    movl    %edx, %ecx
    andl    $3, %ecx            /* ecx = size % 4 */
    rep stosb

    movl    8(%esp), %eax       /* return dst */

    popl    %edi
    ret
.size memset, .-memset

/* --------------------------------------------------------------------------
 * void *memcpy(void *dst, const void *src, size_t size)
 *
 * Stack after push %esi, %edi (2 callee-saves):
 *  12(%esp) = dst   
 *  16(%esp) = src   
 *  20(%esp) = size
 * Returns dst in %eax.
 * -------------------------------------------------------------------------- */
.global memcpy
.type memcpy, @function
memcpy:
    pushl   %esi
    pushl   %edi

    movl    12(%esp), %edi      /* dst */
    movl    16(%esp), %esi      /* src */
    movl    20(%esp), %ecx      /* size */

    cld

    /* Copy dwords */
    movl    %ecx, %edx
    shrl    $2, %ecx            /* ecx = size / 4 */
    rep movsl

    /* Copy remaining bytes */
    movl    %edx, %ecx
    andl    $3, %ecx            /* ecx = size % 4 */
    rep movsb

    movl    12(%esp), %eax      /* return dst */

    popl    %edi
    popl    %esi
    ret
.size memcpy, .-memcpy

/* --------------------------------------------------------------------------
 * int memcmp(const void *s1, const void *s2, size_t n)
 *
 * Stack after push %esi, %edi (2 callee-saves):
 *  12(%esp) = s1   
 *  16(%esp) = s2   
 *  20(%esp) = n
 * Returns <0, 0, or >0 in %eax.
 * -------------------------------------------------------------------------- */
.global memcmp
.type memcmp, @function
memcmp:
    pushl   %esi
    pushl   %edi

    movl    12(%esp), %esi      /* s1 */
    movl    16(%esp), %edi      /* s2 */
    movl    20(%esp), %ecx      /* n  */

    cld

    /* Compare dwords ---------------------------------------------------- */
    movl    %ecx, %edx
    shrl    $2, %ecx            /* ecx = n / 4 */
    repz cmpsl
    jne     .Lcmp_dword_diff    /* mismatch inside a dword */

    /* Compare remaining bytes ------------------------------------------- */
    movl    %edx, %ecx
    andl    $3, %ecx            /* ecx = n % 4 */
    repz cmpsb
    jne     .Lcmp_byte_diff

    /* Equal ------------------------------------------------------------- */
    xorl    %eax, %eax
    jmp     .Lcmp_done

.Lcmp_dword_diff:
    /* esi/edi point one dword past the mismatch.  Back up and
       compare the four bytes individually to get the correct sign.  */
    subl    $4, %esi
    subl    $4, %edi
    movl    $4, %ecx
    repz cmpsb
    /* Fall through – the differing bytes are at -1(%esi) / -1(%edi) */

.Lcmp_byte_diff:
    movzbl  -1(%esi), %eax
    movzbl  -1(%edi), %ecx
    subl    %ecx, %eax

.Lcmp_done:
    popl    %edi
    popl    %esi
    ret
.size memcmp, .-memcmp

.section .multiboot2, "a"
.align 8
mb2_header_start:
    .long 0xE85250D6               # Multiboot2 magic
    .long 0x00000000               # Architecture (i386)
    .long mb2_header_end - mb2_header_start
    .long -(0xE85250D6 + 0x00000000 + (mb2_header_end - mb2_header_start))
    
    # Framebuffer request tag (type=5)
    .align 8
    .short 5                       # Tag type
    .short 0                       # Flags
    .long 20                       # Size of this tag (8-byte aligned)
    .long 1024                     # Width
    .long 768                      # Height
    .long 32                       # Depth (bpp)

    # End tag
    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

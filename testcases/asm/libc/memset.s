    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32
    li a0, 0xDEADBEEF

    sw a0, 0(sp)
    sw a0, 4(sp)
    sw a0, 8(sp)
    sw a0, 12(sp)
    sw a0, 16(sp)
    sw a0, 20(sp)

    mv a0, sp
    li a1, 0
    li a2, 12

    call memset

    lw a1, 0(sp)
    lw a2, 4(sp)
    lw a3, 8(sp)
    lw a4, 12(sp)
    lw a5, 16(sp)
    lw a6, 20(sp)

    addi sp, sp, 32
    lw ra, -4(sp)

    la a0, .str.1
    tail printf

    .rodata
.str.1:
    .string    "%x %x %x %x %x %x\n"

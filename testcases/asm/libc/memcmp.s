    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32
    li a0, 0xabcd1234

    sw a0, 0(sp)
    sw a0, 4(sp)
    sw a0, 8(sp)
    sw a0, 12(sp)
    sw a0, 16(sp)
    sw a0, 20(sp)

    mv a0, sp
    mv a1, sp
    li a2, 32

    call memcmp

    addi sp, sp, 32
    lw ra, -4(sp)

    mv a1, a0
    la a0, .str.1
    tail printf

    .rodata
.str.1:
    .string    "%u\n"

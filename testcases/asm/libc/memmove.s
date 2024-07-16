    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32

    li a0, 24

    call malloc

    mv s0, a0
    la a1, .str.1
    li a2, 24

    call memmove

    li a0, 0

    lw t3, 0(s0)
    add a0, a0, t3
    lw t3, 4(s0)
    add a0, a0, t3
    lw t3, 8(s0)
    add a0, a0, t3
    lw t3, 12(s0)
    add a0, a0, t3
    lw t3, 16(s0)
    add a0, a0, t3
    lw t3, 20(s0)
    add a0, a0, t3

    addi sp, sp, 32
    lw ra, -4(sp)

    mv a1, a0
    la a0, .str.2
    tail printf

    .data
.str.2:
    .string    "%d\n"

.str.1:
    .zero 24
    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32

    li a0, 114514
    call malloc

    li t0, 16
    rem a0, a0, t0

    mv s0, a0

    call free

    mv a0, s0

    addi sp, sp, 32
    lw ra, -4(sp)

    mv a1, a0
    la a0, .str.2
    tail printf

    .data
.str.1:
    .string    "Hello, world!"
.str.2:
    .string    "%d\n"
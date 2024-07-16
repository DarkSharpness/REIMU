    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -16

    li a0, 'H'
    sb a0, 0(sp)
    li a0, 'e'
    sb a0, 1(sp)
    li a0, 'l'
    sb a0, 2(sp)
    li a0, 'l' + 1
    sb a0, 3(sp)
    sb zero, 4(sp)

    la a0, .str.1
    mv a1, sp

    call strcmp

    beqz a0, .end
    bltz a0, .L2
.L1:
    li a0, 1
    j .end
.L2:
    li a0, 2
.end:
    # Expected: 2
    addi a0, a0, -2

    addi sp, sp, 16
    lw ra, -4(sp)

    mv a1, a0
    la a0, .str.2
    tail printf

    .data
.str.1:
    .string    "Hello, world!"
.str.2:
    .string    "%d\n"
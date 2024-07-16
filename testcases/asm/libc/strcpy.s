    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32

    mv a0, sp
    la a1, .str.1

    call strcpy
    addi sp, sp, 32
    lw ra, -4(sp)

    tail puts

    j 0x0 # error!

    .data
    .align    2
.str.1:
    .string    "Hello, world!"
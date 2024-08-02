    .text
    .align    2
    .globl    main
main:
    sw ra, -4(sp)
    addi sp, sp, -32

    li a0, 1
    call malloc
    mv s0, a0

    # la a0, .fmt
    # addi a1, sp, 24
    # call scanf

    # lw a1, 24(sp)

    li a1, 24 # realloc if size > 24
    mv a0, s0

    call realloc

    mv s1, a0
    bne s1, s0, fail
    la a0, .str.2
end:
    addi sp, sp, 32
    lw ra, -4(sp)
    tail puts

fail:
    la a0, .str.1
    j end

    .data
.str.1:
    .string     "fail_to_realloc"
.str.2:
    .string     "success"
.fmt:
    .string     "%d\n"

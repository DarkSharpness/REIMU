    .text
    .align 2
    .globl main
main:
    addi sp, sp, -64
    sw ra, 60(sp)
    sw s0, 56(sp)
    sw s1, 52(sp)
    sw s2, 48(sp)
    sw s3, 44(sp)
    sw s4, 40(sp)
    sw s5, 36(sp)
    sw s6, 32(sp)

    li s6, 0

    # free(NULL) is a no-op.
    li a0, 0
    call free

    li a0, 64
    call malloc
    mv s0, a0

    li a0, 64
    call malloc
    mv s1, a0

    li a0, 64
    call malloc
    mv s2, a0

    # Reuse and split the middle free block.
    mv a0, s1
    call free
    li a0, 16
    call malloc
    mv s3, a0
    beq s3, s1, .reuse_middle_ok
    addi s6, s6, 1
.reuse_middle_ok:
    li a0, 16
    call malloc
    mv s4, a0
    addi t0, s1, 32
    beq s4, t0, .split_ok
    addi s6, s6, 1
.split_ok:

    # Coalesce the split blocks and reuse the combined region.
    mv a0, s3
    call free
    mv a0, s4
    call free
    li a0, 64
    call malloc
    mv s5, a0
    beq s5, s1, .coalesce_ok
    addi s6, s6, 1
.coalesce_ok:

    # Releasing the trailing allocations contracts the heap.
    mv a0, s5
    call free
    mv a0, s2
    call free
    mv a0, s0
    call free
    li a0, 64
    call malloc
    mv s5, a0
    beq s5, s0, .contraction_ok
    addi s6, s6, 1
.contraction_ok:
    mv a0, s5
    call free

    # Growing realloc preserves data even when sbrk relocates host storage.
    li a0, 4
    call malloc
    mv s0, a0
    li t0, 0x12345678
    sw t0, 0(s0)
    mv a0, s0
    li a1, 200000
    call realloc
    mv s0, a0
    li t0, 0x12345678
    lw t1, 0(s0)
    beq t0, t1, .realloc_ok
    addi s6, s6, 1
.realloc_ok:
    mv a0, s0
    call free

    la a0, .format
    mv a1, s6
    call printf

    mv a0, s6
    lw s6, 32(sp)
    lw s5, 36(sp)
    lw s4, 40(sp)
    lw s3, 44(sp)
    lw s2, 48(sp)
    lw s1, 52(sp)
    lw s0, 56(sp)
    lw ra, 60(sp)
    addi sp, sp, 64
    ret

    .data
.format:
    .string "%d\n"

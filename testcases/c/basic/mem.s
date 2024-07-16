	.file	"mem.c"
	.option pic
	.text
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"sum 1..100 = %d\n"
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-16
	li	a0,400
	sw	s0,8(sp)
	sw	ra,12(sp)
	call	malloc
	mv	s0,a0
	mv	a5,a0
	mv	a3,a0
	li	a4,0
	li	a2,100
.L2:
	sw	a4,0(a3)
	addi	a4,a4,1
	addi	a3,a3,4
	bne	a4,a2,.L2
	addi	a3,s0,400
	li	a1,0
.L3:
	lw	a4,0(a5)
	addi	a5,a5,4
	addi	a4,a4,1
	sw	a4,-4(a5)
	add	a1,a1,a4
	bne	a5,a3,.L3
	lla	a0,.LC0
	call	printf
	mv	a0,s0
	call	free
	lw	ra,12(sp)
	lw	s0,8(sp)
	li	a0,0
	addi	sp,sp,16
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits

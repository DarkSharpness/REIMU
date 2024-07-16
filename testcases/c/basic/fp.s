	.file	"fp.c"
	.option pic
	.text
	.align	2
	.globl	add
	.type	add, @function
add:
	add	a0,a0,a1
	ret
	.size	add, .-add
	.align	2
	.globl	sub
	.type	sub, @function
sub:
	sub	a0,a0,a1
	ret
	.size	sub, .-sub
	.align	2
	.globl	mul
	.type	mul, @function
mul:
	mul	a0,a0,a1
	ret
	.size	mul, .-mul
	.align	2
	.globl	div
	.type	div, @function
div:
	div	a0,a0,a1
	ret
	.size	div, .-div
	.align	2
	.globl	run
	.type	run, @function
run:
	li	a5,43
	lla	a0,add
	beq	a2,a5,.L6
	li	a5,45
	lla	a0,sub
	beq	a2,a5,.L6
	li	a5,42
	lla	a0,mul
	beq	a2,a5,.L6
	lla	a0,div
.L6:
	ret
	.size	run, .-run
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"%d %c %d"
	.align	2
.LC1:
	.string	"%d %c %d = %d\n"
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-32
	addi	a3,sp,12
	addi	a2,sp,7
	addi	a1,sp,8
	lla	a0,.LC0
	sw	s0,24(sp)
	sw	s1,20(sp)
	sw	s2,16(sp)
	sw	ra,28(sp)
	call	scanf
	lbu	s0,7(sp)
	li	a4,43
	lw	s1,8(sp)
	lw	s2,12(sp)
	lla	a5,add
	beq	s0,a4,.L13
	li	a4,45
	beq	s0,a4,.L15
	li	a4,42
	beq	s0,a4,.L16
	li	a4,47
	beq	s0,a4,.L18
.L13:
	mv	a1,s2
	mv	a0,s1
	jalr	a5
	mv	a4,a0
	mv	a3,s2
	mv	a2,s0
	mv	a1,s1
	lla	a0,.LC1
	call	printf
	lw	ra,28(sp)
	lw	s0,24(sp)
	lw	s1,20(sp)
	lw	s2,16(sp)
	li	a0,0
	addi	sp,sp,32
	jr	ra
.L16:
	lla	a5,mul
	j	.L13
.L18:
	lla	a5,div
	j	.L13
.L15:
	lla	a5,sub
	j	.L13
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits

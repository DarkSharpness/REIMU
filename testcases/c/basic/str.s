	.file	"str.c"
	.option pic
	.text
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"%s%s"
	.align	2
.LC1:
	.string	"strcmp %s %c %s\n"
	.text
	.align	2
	.globl	test_strcmp
	.type	test_strcmp, @function
test_strcmp:
	addi	sp,sp,-48
	sw	s0,40(sp)
	sw	s1,36(sp)
	addi	s0,sp,12
	addi	s1,sp,22
	mv	a2,s1
	mv	a1,s0
	lla	a0,.LC0
	sw	ra,44(sp)
	call	scanf
	mv	a1,s1
	mv	a0,s0
	call	strcmp
	li	a2,60
	blt	a0,zero,.L2
	snez	a2,a0
	addi	a2,a2,61
.L2:
	mv	a3,s1
	mv	a1,s0
	lla	a0,.LC1
	call	printf
	lw	ra,44(sp)
	lw	s0,40(sp)
	lw	s1,36(sp)
	addi	sp,sp,48
	jr	ra
	.size	test_strcmp, .-test_strcmp
	.section	.rodata.str1.4
	.align	2
.LC2:
	.string	"%s"
	.align	2
.LC3:
	.string	"strcpy %s -> %s\n"
	.text
	.align	2
	.globl	test_strcpy
	.type	test_strcpy, @function
test_strcpy:
	addi	sp,sp,-64
	sw	s0,56(sp)
	addi	s0,sp,8
	mv	a1,s0
	lla	a0,.LC2
	sw	ra,60(sp)
	call	scanf
	addi	a2,sp,28
	mv	a1,s0
	mv	a0,a2
	call	strcpy
	mv	a2,a0
	mv	a1,s0
	lla	a0,.LC3
	call	printf
	lw	ra,60(sp)
	lw	s0,56(sp)
	addi	sp,sp,64
	jr	ra
	.size	test_strcpy, .-test_strcpy
	.section	.rodata.str1.4
	.align	2
.LC4:
	.string	"strcat %s + %s = %s\n"
	.text
	.align	2
	.globl	test_strcat
	.type	test_strcat, @function
test_strcat:
	addi	sp,sp,-96
	sw	s0,88(sp)
	sw	s1,84(sp)
	mv	s0,sp
	addi	s1,sp,20
	mv	a2,s1
	mv	a1,s0
	lla	a0,.LC0
	sw	ra,92(sp)
	call	scanf
	addi	a3,sp,40
	mv	a1,s0
	mv	a0,a3
	call	strcpy
	mv	a1,s1
	call	strcat
	mv	a3,a0
	mv	a2,s1
	mv	a1,s0
	lla	a0,.LC4
	call	printf
	lw	ra,92(sp)
	lw	s0,88(sp)
	lw	s1,84(sp)
	addi	sp,sp,96
	jr	ra
	.size	test_strcat, .-test_strcat
	.section	.rodata.str1.4
	.align	2
.LC5:
	.string	"strlen %s = %d\n"
	.text
	.align	2
	.globl	test_strlen
	.type	test_strlen, @function
test_strlen:
	addi	sp,sp,-48
	sw	s0,40(sp)
	addi	s0,sp,12
	mv	a1,s0
	lla	a0,.LC2
	sw	ra,44(sp)
	call	scanf
	mv	a0,s0
	call	strlen
	mv	a2,a0
	mv	a1,s0
	lla	a0,.LC5
	call	printf
	lw	ra,44(sp)
	lw	s0,40(sp)
	addi	sp,sp,48
	jr	ra
	.size	test_strlen, .-test_strlen
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-112
	sw	ra,108(sp)
	sw	s0,104(sp)
	sw	s1,100(sp)
	sw	s2,96(sp)
	sw	s3,92(sp)
	sw	s4,88(sp)
	sw	s5,84(sp)
	li	s1,3
	call	test_strcmp
	addi	s0,sp,40
	call	test_strcmp
	addi	s2,sp,60
	call	test_strcmp
	lla	s4,.LC2
	lla	s3,.LC3
.L15:
	mv	a1,s0
	mv	a0,s4
	call	scanf
	mv	a1,s0
	mv	a0,s2
	call	strcpy
	addi	s1,s1,-1
	mv	a2,s2
	mv	a1,s0
	mv	a0,s3
	call	printf
	bne	s1,zero,.L15
	li	s3,3
	mv	s2,sp
	addi	s1,sp,20
	lla	s5,.LC0
	lla	s4,.LC4
.L16:
	mv	a2,s1
	mv	a1,s2
	mv	a0,s5
	call	scanf
	mv	a1,s2
	mv	a0,s0
	call	strcpy
	mv	a1,s1
	mv	a0,s0
	call	strcat
	addi	s3,s3,-1
	mv	a3,s0
	mv	a2,s1
	mv	a1,s2
	mv	a0,s4
	call	printf
	bne	s3,zero,.L16
	li	s1,3
	lla	s3,.LC2
	lla	s2,.LC5
.L17:
	mv	a1,s0
	mv	a0,s3
	call	scanf
	mv	a0,s0
	call	strlen
	mv	a2,a0
	addi	s1,s1,-1
	mv	a1,s0
	mv	a0,s2
	call	printf
	bne	s1,zero,.L17
	lw	ra,108(sp)
	lw	s0,104(sp)
	lw	s1,100(sp)
	lw	s2,96(sp)
	lw	s3,92(sp)
	lw	s4,88(sp)
	lw	s5,84(sp)
	li	a0,0
	addi	sp,sp,112
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits

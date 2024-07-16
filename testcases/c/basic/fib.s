	.file	"fib.c"
	.option pic
	.text
	.align	2
	.globl	fib
	.type	fib, @function
fib:
	li	a4,1
	bleu	a0,a4,.L46
	addi	sp,sp,-128
	sw	s0,120(sp)
	sw	s9,84(sp)
	sw	ra,124(sp)
	sw	s1,116(sp)
	sw	s2,112(sp)
	sw	s3,108(sp)
	sw	s4,104(sp)
	sw	s5,100(sp)
	sw	s6,96(sp)
	sw	s7,92(sp)
	sw	s8,88(sp)
	sw	s10,80(sp)
	sw	s11,76(sp)
	addi	s9,a0,-1
	li	a3,0
	li	s0,1
.L4:
	bleu	s9,s0,.L47
	addi	a7,s9,-1
	mv	s10,a7
	li	a2,0
.L7:
	beq	s10,s0,.L48
	addi	t1,s10,-1
	mv	s11,t1
	li	a1,0
.L10:
	beq	s11,s0,.L49
	addi	t3,s11,-1
	mv	a4,t3
	li	a6,0
.L13:
	beq	a4,s0,.L50
	addi	t4,a4,-1
	mv	s2,t4
	li	s7,0
.L16:
	beq	s2,s0,.L51
	addi	s8,s2,-1
	mv	a5,s8
	li	s3,0
.L19:
	beq	a5,s0,.L52
	addi	t5,a5,-2
	addi	a5,a5,-1
	mv	t6,t5
	mv	t0,a5
	li	s5,0
	mv	s1,t6
	li	s4,0
	bleu	t0,s0,.L53
.L21:
	mv	t2,s1
	li	s6,0
	bleu	s1,s0,.L54
.L24:
	addi	a0,t2,-1
	sw	a5,60(sp)
	sw	t5,56(sp)
	sw	t3,52(sp)
	sw	t4,48(sp)
	sw	t1,44(sp)
	sw	a7,40(sp)
	sw	t0,36(sp)
	sw	t6,32(sp)
	sw	a4,28(sp)
	sw	a6,24(sp)
	sw	a1,20(sp)
	sw	a2,16(sp)
	sw	a3,12(sp)
	sw	t2,8(sp)
	call	fib
	lw	t2,8(sp)
	lw	a3,12(sp)
	lw	a2,16(sp)
	addi	t2,t2,-2
	lw	a1,20(sp)
	lw	a6,24(sp)
	lw	a4,28(sp)
	lw	t6,32(sp)
	lw	t0,36(sp)
	lw	a7,40(sp)
	lw	t1,44(sp)
	lw	t4,48(sp)
	lw	t3,52(sp)
	lw	t5,56(sp)
	lw	a5,60(sp)
	add	s6,s6,a0
	bgtu	t2,s0,.L24
	addi	s6,s6,1
.L26:
	addi	a0,s1,-1
	add	s4,s4,s6
	addi	s1,s1,-2
	bgtu	a0,s0,.L21
	addi	s4,s4,1
	add	s5,s5,s4
	addi	a0,t6,-2
	addi	t0,t0,-2
	bleu	t6,s0,.L55
.L34:
	mv	t6,a0
	mv	s1,t6
	li	s4,0
	bgtu	t0,s0,.L21
.L53:
	li	s4,1
	add	s5,s5,s4
	addi	a0,t6,-2
	addi	t0,t0,-2
	bgtu	t6,s0,.L34
.L55:
	addi	s5,s5,1
	add	s3,s3,s5
	bgtu	a5,s0,.L32
.L56:
	addi	s3,s3,1
.L20:
	add	s7,s7,s3
	addi	s2,s2,-2
	bgtu	s8,s0,.L16
	addi	s7,s7,1
.L17:
	add	a6,a6,s7
	addi	a4,a4,-2
	bgtu	t4,s0,.L13
	addi	a6,a6,1
.L14:
	add	a1,a1,a6
	addi	s11,s11,-2
	bgtu	t3,s0,.L10
	addi	a1,a1,1
.L11:
	add	a2,a2,a1
	addi	s10,s10,-2
	bgtu	t1,s0,.L7
	addi	a2,a2,1
.L8:
	add	a3,a3,a2
	addi	s9,s9,-2
	bgtu	a7,s0,.L4
	lw	ra,124(sp)
	lw	s0,120(sp)
	lw	s1,116(sp)
	lw	s2,112(sp)
	lw	s3,108(sp)
	lw	s4,104(sp)
	lw	s5,100(sp)
	lw	s6,96(sp)
	lw	s7,92(sp)
	lw	s8,88(sp)
	lw	s9,84(sp)
	lw	s10,80(sp)
	lw	s11,76(sp)
	addi	a0,a3,1
	addi	sp,sp,128
	jr	ra
.L52:
	li	s5,1
	addi	a5,s0,-1
	addi	t5,s0,-2
	add	s3,s3,s5
	bleu	a5,s0,.L56
.L32:
	mv	a5,t5
	j	.L19
.L54:
	li	s6,1
	j	.L26
.L51:
	li	s3,1
	addi	s8,s2,-1
	j	.L20
.L50:
	li	s7,1
	addi	t4,a4,-1
	j	.L17
.L49:
	li	a6,1
	addi	t3,s11,-1
	j	.L14
.L48:
	li	a1,1
	addi	t1,s10,-1
	j	.L11
.L47:
	li	a2,1
	addi	a7,s9,-1
	j	.L8
.L46:
	li	a0,1
	ret
	.size	fib, .-fib
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC0:
	.string	"%d"
	.align	2
.LC1:
	.string	"fib(%d) = %d\n"
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-48
	addi	a1,sp,12
	lla	a0,.LC0
	sw	s1,36(sp)
	sw	s2,32(sp)
	sw	s3,28(sp)
	sw	s4,24(sp)
	sw	ra,44(sp)
	sw	s0,40(sp)
	call	scanf
	lw	a5,12(sp)
	li	s2,0
	li	s1,0
	lla	s4,.LC1
	li	s3,1
	blt	a5,zero,.L59
.L58:
	mv	a1,s2
	addi	a2,s1,1
	mv	a0,s4
	call	printf
	lw	a5,12(sp)
	addi	s2,s2,1
	blt	a5,s2,.L59
.L60:
	beq	s2,s3,.L64
	mv	s0,s2
	li	s1,0
.L62:
	addi	a0,s0,-1
	call	fib
	addi	s0,s0,-2
	add	s1,s1,a0
	bgt	s0,s3,.L62
	mv	a1,s2
	addi	a2,s1,1
	mv	a0,s4
	call	printf
	lw	a5,12(sp)
	addi	s2,s2,1
	bge	a5,s2,.L60
.L59:
	lw	ra,44(sp)
	lw	s0,40(sp)
	lw	s1,36(sp)
	lw	s2,32(sp)
	lw	s3,28(sp)
	lw	s4,24(sp)
	li	a0,0
	addi	sp,sp,48
	jr	ra
.L64:
	li	s1,0
	j	.L58
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits

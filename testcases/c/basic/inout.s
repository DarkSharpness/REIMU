	.file	"inout.c"
	.option pic
	.text
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align	2
.LC1:
	.string	"%s"
	.align	2
.LC0:
	.string	"Hello, World!"
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
	lla	a5,.LC0
	addi	sp,sp,-32
	lw	a2,0(a5)
	lw	a3,4(a5)
	lw	a4,8(a5)
	lhu	a5,12(a5)
	sw	s0,24(sp)
	mv	s0,sp
	mv	a0,s0
	sw	ra,28(sp)
	sw	a2,0(sp)
	sw	a3,4(sp)
	sw	a4,8(sp)
	sh	a5,12(sp)
	call	puts
	mv	a1,s0
	lla	a0,.LC1
	call	scanf
	mv	a0,s0
	call	puts
	lw	ra,28(sp)
	lw	s0,24(sp)
	li	a0,0
	addi	sp,sp,32
	jr	ra
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits

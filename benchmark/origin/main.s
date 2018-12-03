	.set noreorder
	.set volatile
	.set noat
	.set nomacro
	.arch ev6
	.section	.rodata
$LC0:
	.ascii "address a : %p\12address a : %p\12address i : %p\12\0"
$LC1:
	.ascii "a[%d]  %d\12\0"
	.text
	.align 2
	.globl main
	.ent main
main:
	.frame $15,96,$26,0
	.mask 0x4008000,-96
	ldah $29,0($27)		!gpdisp!1
	lda $29,0($29)		!gpdisp!1
$main..ng:
	lda $30,-96($30)
	stq $26,0($30)
	stq $15,8($30)
	mov $30,$15
	.prologue 1
	ldah $1,$LC0($29)		!gprelhigh
	lda $3,$LC0($1)		!gprellow
	lda $2,1040($15)
	lda $1,80($15)
	mov $3,$16
	lda $17,16($15)
	mov $2,$18
	mov $1,$19
	ldq $27,__nldbl_printf($29)		!literal!2
	jsr $26,($27),0		!lituse_jsr!2
	ldah $29,0($26)		!gpdisp!3
	lda $29,0($29)		!gpdisp!3
	stl $31,80($15)
	br $31,$L2
$L3:
	ldl $1,80($15)
	ldl $2,80($15)
	s4addq $1,0,$1
	lda $3,16($15)
	addq $3,$1,$1
	stl $2,0($1)
	ldah $1,$LC1($29)		!gprelhigh
	lda $3,$LC1($1)		!gprellow
	ldl $2,80($15)
	ldl $1,80($15)
	s4addq $1,0,$1
	lda $4,16($15)
	addq $4,$1,$1
	ldl $1,0($1)
	mov $3,$16
	mov $2,$17
	mov $1,$18
	ldq $27,__nldbl_printf($29)		!literal!4
	jsr $26,($27),0		!lituse_jsr!4
	ldah $29,0($26)		!gpdisp!5
	lda $29,0($29)		!gpdisp!5
	ldl $1,80($15)
	addl $1,1,$1
	addl $31,$1,$1
	stl $1,80($15)
$L2:
	ldl $1,80($15)
	cmple $1,15,$1
	bne $1,$L3
	mov $15,$30
	ldq $26,0($30)
	ldq $15,8($30)
	lda $30,96($30)
	ret $31,($26),1
	.end main
	.ident	"GCC: (GNU) 4.5.3"
	.section	.note.GNU-stack,"",@progbits

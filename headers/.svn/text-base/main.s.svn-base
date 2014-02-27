	.file	"main.c"
	.section	.rodata
.LC0:
	.string	"YOU ROCK!!"
	.text
.globl target_func1
	.type	target_func1, @function
target_func1:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$.LC0, (%esp)
	call	puts
	leave
	ret
	.size	target_func1, .-target_func1
	.section	.rodata
.LC1:
	.string	"WOW"
	.text
.globl target_func2
	.type	target_func2, @function
target_func2:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$.LC1, (%esp)
	call	puts
	leave
	ret
	.size	target_func2, .-target_func2
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	pushl	%esi
	pushl	%ebx
	subl	$24, %esp
	movl	$target_func1, 8(%esp)
	movl	$target_func2, 4(%esp)
	movl	12(%esp), %edx
	movl	8(%esp), %ecx
	movl	4(%esp), %esi
#APP
# 44 "main.c" 1
	pusha
	call test_get_eip
	test_get_eip:
	pop %eax
	movl %eax, %edx
	movl %ecx %ebx
	pushl 16(%edx)
	jmp (%ebx)
	popa
	
# 0 "" 2
#NO_APP
	movl	%edx, 12(%esp)
	call	target_func2
	movl	$0, %eax
	addl	$24, %esp
	popl	%ebx
	popl	%esi
	movl	%ebp, %esp
	popl	%ebp
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu 4.4.3-4ubuntu5) 4.4.3"
	.section	.note.GNU-stack,"",@progbits

	.global _magic
	.global magic
_magic:
magic:
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15

	mov %rdi,%r15

	call _current

	mov (%rax),%rbx
	mov %r15,(%rax)
	mov %rsp,(%rbx)
	mov (%r15),%rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	
	ret


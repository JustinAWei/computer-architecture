	.global start
start:
	subs x1,x1,#1
        mov x2,#'x'
	strb w2,[x1]
	mov x2,#10
	strb w2,[x1]
	ret


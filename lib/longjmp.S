	.text
	.globl longjmp
longjmp:
	mov r2, r0
	mov r0, r1
	mov r1, r2
	cmp r0, #0
	mov.eq r0, #1
	ld r2, (r1+8)
	ld r3, (r1+12)
	ld r4, (r1+16)
	ld r5, (r1+20)
	ld r6, (r1+24)
	ld r7, (r1+28)
	ld r8, (r1+32)
	ld r9, (r1+36)
	ld r10, (r1+40)
	ld r11, (r1+44)
	ld r12, (r1+48)
	ld r13, (r1+52)
	ld r14, (r1+56)
	ld r15, (r1+60)
	ld r16, (r1+64)
	ld r17, (r1+68)
	ld r18, (r1+72)
	ld r19, (r1+76)
	ld r20, (r1+80)
	ld r21, (r1+84)
	ld r22, (r1+88)
	ld r23, (r1+92)
	ld gp, (r1+96)
	ld sp, (r1+100)
	ld lr, (r1+104)
	ld r1, (r1+4)
	rts

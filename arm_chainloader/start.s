.text
.globl _start
_start:
	/* vectors */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	/* comm chan */
	nop
	nop
	nop
	nop

	mov sp, #0x2000000
	b _firmware_rendezvous

L_deadloop:
	b L_deadloop

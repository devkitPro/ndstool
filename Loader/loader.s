@
@	Small NDS loader for PassMe/WifiMe/FlashMe
@	for directly flashing an NDS to GBA flashcart
@	This loader replaces the logo data in the NDS header.
@	Note that there's no filesystem support.
@

	.equ	NewARM9Loop_dest,	0x027FFFF0
	.equ	RAM_HEADER,		0x027FFE00


@ start offset is 0xC0

	.arm
	.global	_start
_start:
	adr	r6, _start - 0xC0			@ NDS header
	adr	r7, ThumbCode+1
	bx	r7

	.thumb
ThumbCode:

@	@ clear RAM
@	mov	r0, #0
@	mov	r1, #0
@	mov	r2, #0
@	mov	r3, #0
@	ldr	r4, =0x02000000
@	ldr	r5, =0x023FF000
@1:
@	stmia	r4!, {r0-r3}
@	cmp	r4, r5
@	bne	1b

	@
	ldr	r7, =RAM_HEADER

	@ copy new ARM9 loop
	ldr	r0, NewARM9Loop
	ldr	r4, =NewARM9Loop_dest
	str	r0, [r4, #0]				@ place ldr instruction
	str	r4, [r4, #4]				@ address of ldr instruction
	str	r4, [r7, #0x24]				@ go to new loop

	@ copy header
	mov	r0, r6
	mov	r1, r7
	ldr	r2, =0x1F0				@ do not overwrite new ARM9 loop
	bl	Copy

	@ copy ARM9 binary
	ldr	r0, [r6, #0x20]				@ ROM offset
	add	r0, r0, r6
	ldr	r1, [r6, #0x28]				@ RAM address
	ldr	r2, [r6, #0x2C]				@ code size
	bl	CopyAlign

	@ copy ARM7 binary
	ldr	r0, [r6, #0x30]				@ ROM offset
	add	r0, r0, r6
	ldr	r1, [r6, #0x38]				@ RAM address
	ldr	r2, [r6, #0x3C]				@ code size
	bl	Copy

	@ get ARM9/7 entry
	ldr	r0, [r6, #0x24]
	ldr	r7, [r6, #0x34]

	@ start ARM9
	ldr	r4, =NewARM9Loop_dest
	str	r0, [r4, #0x4]

	bx	r7


@ copy [r0+] to [r1+]; length r2; destroys r3

CopyAlign:
	ldr	r3, =0x1FF
	add	r2, r2, r3
	bic	r2, r2, r3
Copy:
	@ start DMA
@	ldr	r3, =0x040000D4		@ DMA3
@	str	r0, [r3, #0]		@ src
@	str	r1, [r3, #4]		@ dest
	ldr	r3, =1<<26
	lsr	r2, r2, #2		@ 32 bits
	add	r2, r2, r3
@	str	r2, [r3, #8]		@ control
	swi	0x0c			@ CpuFastSet
	mov	pc, lr


	.align
	.arm

NewARM9Loop:
	ldr		pc, . + 4

	.pool

	.space (_start + 156) - .

	.end

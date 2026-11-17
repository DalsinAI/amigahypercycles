/* hc_fixpoint.c: FIXPOINT.ASM's five routines in C, giving the same bits as
 * the 386 code (Watcom passes the arguments in EAX and EDX):
 *   fixmul   (a * b) >> 16   the 64-bit product's bits 16..47 (IMUL, SHLD 16)
 *   fixmul2  (a * b) >> 8    bits 8..39 (SHLD 24)
 *   fixdiv   (a << 16) / b   0 when a or b is 0 (SAR/SHL, IDIV)
 *   shf1     a >> 22         unsigned (SHLD 10 into a cleared EDX)
 *   qrshift  a >> n          unsigned (SHR EAX, CL)
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include <stdint.h>

int fixmul(int a, int b)  { return (int)(uint32_t)((uint64_t)((int64_t)a * b) >> 16); }
int fixmul2(int a, int b) { return (int)(uint32_t)((uint64_t)((int64_t)a * b) >> 8); }

int fixdiv(int a, int b)
{
    int64_t q;
    if (a == 0 || b == 0) return 0;
    q = ((int64_t)a * 65536) / b;
    return (int)(uint32_t)(uint64_t)q;
}

int shf1(int a)           { return (int)((uint32_t)a >> 22); }
int qrshift(int a, int n) { return (int)((uint32_t)a >> (n & 31)); }

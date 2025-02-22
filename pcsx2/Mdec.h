/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __MDEC_H__
#define __MDEC_H__

// MDEC status:
#define MDEC_BUSY	0x20000000
#define MDEC_DREQ	0x18000000
#define MDEC_FIFO	0xc0000000
#define MDEC_RGB24	0x02000000
#define MDEC_STP	0x00800000

#define CONST_BITS		8
#define PASS1_BITS		2
#define CONST_BITS14		14
#define	IFAST_SCALE_BITS	2

#define FIX_1_082392200  (277)
#define FIX_1_414213562  (362)
#define FIX_1_847759065  (473)
#define FIX_2_613125930  (669)

#define MULTIPLY(var,const)  (DESCALE((var) * (const), CONST_BITS))

#define DEQUANTIZE(coef,quantval)  (coef)

#define DESCALE(x,n)  ((x)>>(n))
#define RANGE(n) (n)

#define DCTSIZE 8
#define DCTSIZE2 64

#define RUNOF(a) ((a)>>10)
#define VALOF(a) (((int)(a)<<(32-10))>>(32-10))
#define NOP	0xfe00

#define	MULR(a)		((((int)0x0000059B) * (a)) >> 10)
#define	MULG(a)		((((int)0xFFFFFEA1) * (a)) >> 10)
#define	MULG2(a)	((((int)0xFFFFFD25) * (a)) >> 10)
#define	MULB(a)		((((int)0x00000716) * (a)) >> 10)

#define	MAKERGB15(r,g,b)	( (((r)>>3)<<10)|(((g)>>3)<<5)|((b)>>3) )
#define	ROUND(c)	roundtbl[((c)+128+256)]//&0x3ff]

#define RGB15(n, Y) \
	image[n] = MAKERGB15(ROUND(Y + R),ROUND(Y + G),ROUND(Y + B));

#define RGB15BW(n, Y) \
	image[n] = MAKERGB15(ROUND(Y),ROUND(Y),ROUND(Y));

#define RGB24(n, Y) \
	image[n+2] = ROUND(Y + R); \
	image[n+1] = ROUND(Y + G); \
	image[n+0] = ROUND(Y + B);

#define RGB24BW(n, Y) \
	image[n+2] = ROUND(Y); \
	image[n+1] = ROUND(Y); \
	image[n+0] = ROUND(Y);

extern void mdecInit(void);
extern void mdecWrite0(u32 data);
extern void mdecWrite1(u32 data);
extern u32  mdecRead0(void);
extern u32  mdecRead1(void);
extern void psxDma0(u32 madr, u32 bcr, u32 chcr);
extern void psxDma1(u32 madr, u32 bcr, u32 chcr);

#endif /* __MDEC_H__ */

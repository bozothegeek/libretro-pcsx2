/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ix86 definitions v0.9.1
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.1:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

#pragma once

#include <vector>

namespace x86Emitter
{
extern void xStoreReg(const xRegisterSSE &src);
extern void xRestoreReg(const xRegisterSSE &dest);

// ------------------------------------------------------------------------
// Group 1 Instruction Class

extern const xImpl_Group1 xADC;
extern const xImpl_Group1 xSBB;

extern const xImpl_G1Logic xAND;
extern const xImpl_G1Logic xOR;
extern const xImpl_G1Logic xXOR;

extern const xImpl_G1Arith xADD;
extern const xImpl_G1Arith xSUB;
extern const xImpl_G1Compare xCMP;

// ------------------------------------------------------------------------
// Group 2 Instruction Class
//
// Optimization Note: For Imm forms, we ignore the instruction if the shift count is
// zero.  This is a safe optimization since any zero-value shift does not affect any
// flags.

extern const xImpl_Mov xMOV;
#ifdef __M_X86_64
extern const xImpl_MovImm64 xMOV64;
#endif
extern const xImpl_Test xTEST;

extern const xImpl_Group2 xROL, xROR,
    xRCL, xRCR,
    xSHL, xSHR,
    xSAR;

// ------------------------------------------------------------------------
// Group 3 Instruction Class

extern const xImpl_Group3 xNOT, xNEG;
extern const xImpl_Group3 xUMUL, xUDIV;
extern const xImpl_iDiv xDIV;
extern const xImpl_iMul xMUL;

extern const xImpl_IncDec xINC, xDEC;

extern const xImpl_MovExtend xMOVZX, xMOVSX;

extern const xImpl_DwordShift xSHLD, xSHRD;

extern const xImpl_Group8 xBT;
extern const xImpl_Group8 xBTR;
extern const xImpl_Group8 xBTS;
extern const xImpl_Group8 xBTC;

extern const xImpl_BitScan xBSF, xBSR;

extern const xImpl_JmpCall xJMP;
extern const xImpl_JmpCall xCALL;
extern const xImpl_FastCall xFastCall;

// ------------------------------------------------------------------------
extern const xImpl_CMov xCMOVA, xCMOVAE,
    xCMOVB, xCMOVBE,
    xCMOVG, xCMOVGE,
    xCMOVL, xCMOVLE,

    xCMOVZ, xCMOVE,
    xCMOVNZ, xCMOVNE,
    xCMOVO, xCMOVNO,
    xCMOVC, xCMOVNC,

    xCMOVS, xCMOVNS,
    xCMOVPE, xCMOVPO;

// ------------------------------------------------------------------------
extern const xImpl_Set xSETA, xSETAE,
    xSETB, xSETBE,
    xSETG, xSETGE,
    xSETL, xSETLE,

    xSETZ, xSETE,
    xSETNZ, xSETNE,
    xSETO, xSETNO,
    xSETC, xSETNC,

    xSETS, xSETNS,
    xSETPE, xSETPO;

//////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous Instructions
// These are all defined inline or in ix86.cpp.
//

// ----- Lea Instructions (Load Effective Address) -----
// Note: alternate (void*) forms of these instructions are not provided since those
// forms are functionally equivalent to Mov reg,imm, and thus better written as MOVs
// instead.

extern void xLEA(xRegister64 to, const xIndirectVoid &src, bool preserve_flags = false);
extern void xLEA(xRegister32 to, const xIndirectVoid &src, bool preserve_flags = false);
extern void xLEA(xRegister16 to, const xIndirectVoid &src, bool preserve_flags = false);
/// LEA with a target that will be decided later, guarantees that no optimizations are performed that could change what needs to be written in
extern u32* xLEA_Writeback(xAddressReg to);

// ----- Push / Pop Instructions  -----
// Note: pushad/popad implementations are intentionally left out.  The instructions are
// invalid in x64, and are super slow on x32.  Use multiple Push/Pop instructions instead.

extern void xPOP(const xIndirectVoid &from);
extern void xPUSH(const xIndirectVoid &from);

extern void xPOP(xRegister32or64 from);

extern void xPUSH(u32 imm);
extern void xPUSH(xRegister32or64 from);

// ----- Miscellaneous Instructions  -----
// Various Instructions with no parameter and no special encoding logic.

#define xLEAVE() (xWrite8(0xC9))
#define xRET()   (xWrite8(0xC3))
#define xCDQ()   (xWrite8(0x99))
#define xCWDE()  (xWrite8(0x98))
#define xLAHF()  (xWrite8(0x9f))

// NOP 1-byte
#define xNOP()   (xWrite8(0x90))

extern void xINT(u8 imm);

//////////////////////////////////////////////////////////////////////////////////////////
// Helper object to handle the various functions ABI
class xScopedStackFrame
{
    bool m_base_frame;
    bool m_save_base_pointer;
    int m_offset;

public:
    xScopedStackFrame(bool base_frame, bool save_base_pointer = false, int offset = 0);
    ~xScopedStackFrame();
};

//////////////////////////////////////////////////////////////////////////////////////////
/// Helper object to save some temporary registers before the call
class xScopedSavedRegisters
{
    std::vector<std::reference_wrapper<const xAddressReg>> regs;
public:
    xScopedSavedRegisters(std::initializer_list<std::reference_wrapper<const xAddressReg>> regs);
    ~xScopedSavedRegisters();
};

//////////////////////////////////////////////////////////////////////////////////////////
/// Helper function to calculate base+offset taking into account the limitations of x86-64's RIP-relative addressing
/// (Will either return `base+offset` or LEA `base` into `tmpRegister` and return `tmpRegister+offset`)
xAddressVoid xComplexAddress(const xAddressReg& tmpRegister, void *base, const xAddressVoid& offset);

//////////////////////////////////////////////////////////////////////////////////////////
/// Helper function to load addresses that may be far from the current instruction pointer
/// On i386, resolves to `mov dst, (sptr)addr`
/// On x86-64, resolves to either `mov dst, (sptr)addr` or `lea dst, [addr]` depending on the distance from RIP
void xLoadFarAddr(const xAddressReg& dst, void *addr);

//////////////////////////////////////////////////////////////////////////////////////////
// JMP / Jcc Instructions!

extern void xJcc(JccComparisonType comparison, const void *target);
extern s32 *xJcc32(JccComparisonType comparison = Jcc_Unconditional, s32 displacement = 0);

// ------------------------------------------------------------------------
// Conditional jumps to fixed targets.
// Jumps accept any pointer as a valid target (function or data), and will generate either
// 8 or 32 bit displacement versions of the jump, depending on relative displacement of
// the target (efficient!)
//

template <typename T>
__fi void xJNE(T *func)
{
    xJcc(Jcc_NotEqual, (void *)(uptr)func);
}
template <typename T>
__fi void xJNZ(T *func)
{
    xJcc(Jcc_NotZero, (void *)(uptr)func);
}

template <typename T>
__fi void xJC(T *func)
{
    xJcc(Jcc_Carry, (void *)(uptr)func);
}
template <typename T>
__fi void xJS(T *func)
{
    xJcc(Jcc_Signed, (void *)(uptr)func);
}

template <typename T>
__fi void xJL(T *func)
{
    xJcc(Jcc_Less, (void *)(uptr)func);
}
template <typename T>
__fi void xJLE(T *func)
{
    xJcc(Jcc_LessOrEqual, (void *)(uptr)func);
}

// ------------------------------------------------------------------------
// Forward Jump Helpers (act as labels!)

#define DEFINE_FORWARD_JUMP(label, cond)                     \
    template <typename OperandType>                          \
    class xForward##label : public xForwardJump<OperandType> \
    {                                                        \
    public:                                                  \
        xForward##label()                                    \
            : xForwardJump<OperandType>(cond)                \
        {                                                    \
        }                                                    \
    };

// ------------------------------------------------------------------------
// Note: typedefs below  are defined individually in order to appease Intellisense
// resolution.  Including them into the class definition macro above breaks it.

typedef xForwardJump<s8> xForwardJump8;
typedef xForwardJump<s32> xForwardJump32;

DEFINE_FORWARD_JUMP(JA, Jcc_Above);
DEFINE_FORWARD_JUMP(JB, Jcc_Below);
DEFINE_FORWARD_JUMP(JAE, Jcc_AboveOrEqual);
DEFINE_FORWARD_JUMP(JBE, Jcc_BelowOrEqual);

typedef xForwardJA<s8> xForwardJA8;
typedef xForwardJB<s8> xForwardJB8;
typedef xForwardJB<s32> xForwardJB32;
typedef xForwardJAE<s8> xForwardJAE8;
typedef xForwardJBE<s8> xForwardJBE8;
typedef xForwardJBE<s32> xForwardJBE32;

DEFINE_FORWARD_JUMP(JG, Jcc_Greater);
DEFINE_FORWARD_JUMP(JL, Jcc_Less);
DEFINE_FORWARD_JUMP(JGE, Jcc_GreaterOrEqual);
DEFINE_FORWARD_JUMP(JLE, Jcc_LessOrEqual);

typedef xForwardJG<s8> xForwardJG8;
typedef xForwardJG<s32> xForwardJG32;
typedef xForwardJL<s8> xForwardJL8;
typedef xForwardJL<s32> xForwardJL32;
typedef xForwardJGE<s8> xForwardJGE8;
typedef xForwardJGE<s32> xForwardJGE32;
typedef xForwardJLE<s8> xForwardJLE8;
typedef xForwardJLE<s32> xForwardJLE32;

DEFINE_FORWARD_JUMP(JZ, Jcc_Zero);
DEFINE_FORWARD_JUMP(JE, Jcc_Equal);
DEFINE_FORWARD_JUMP(JNZ, Jcc_NotZero);
DEFINE_FORWARD_JUMP(JNE, Jcc_NotEqual);

typedef xForwardJZ<s8> xForwardJZ8;
typedef xForwardJZ<s32> xForwardJZ32;
typedef xForwardJE<s8> xForwardJE8;
typedef xForwardJE<s32> xForwardJE32;
typedef xForwardJNZ<s8> xForwardJNZ8;
typedef xForwardJNZ<s32> xForwardJNZ32;
typedef xForwardJNE<s8> xForwardJNE8;
typedef xForwardJNE<s32> xForwardJNE32;

DEFINE_FORWARD_JUMP(JS, Jcc_Signed);
DEFINE_FORWARD_JUMP(JNS, Jcc_Unsigned);

typedef xForwardJNS<s8> xForwardJNS8;

DEFINE_FORWARD_JUMP(JO, Jcc_Overflow);
DEFINE_FORWARD_JUMP(JNO, Jcc_NotOverflow);

DEFINE_FORWARD_JUMP(JC, Jcc_Carry);
DEFINE_FORWARD_JUMP(JNC, Jcc_NotCarry);

typedef xForwardJC<s8> xForwardJC8;
typedef xForwardJC<s32> xForwardJC32;
typedef xForwardJNC<s8> xForwardJNC8;
typedef xForwardJNC<s32> xForwardJNC32;

DEFINE_FORWARD_JUMP(JPE, Jcc_ParityEven);
DEFINE_FORWARD_JUMP(JPO, Jcc_ParityOdd);

typedef xForwardJPE<s8> xForwardJPE8;
typedef xForwardJPE<s32> xForwardJPE32;
typedef xForwardJPO<s8> xForwardJPO8;
typedef xForwardJPO<s32> xForwardJPO32;

// ------------------------------------------------------------------------

extern void xLDMXCSR(const xIndirect32 &src);

extern void xMOVDZX(const xRegisterSSE &to, const xRegister32or64 &from);
extern void xMOVDZX(const xRegisterSSE &to, const xIndirectVoid &src);

extern void xMOVD(const xRegister32or64 &to, const xRegisterSSE &from);
extern void xMOVD(const xIndirectVoid &dest, const xRegisterSSE &from);

extern void xMOVQ(const xIndirectVoid &dest, const xRegisterSSE &from);

extern void xMOVQZX(const xRegisterSSE &to, const xIndirectVoid &src);
extern void xMOVQZX(const xRegisterSSE &to, const xRegisterSSE &from);

extern void xMOVSS(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xMOVSS(const xIndirectVoid &to, const xRegisterSSE &from);
extern void xMOVSD(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xMOVSD(const xIndirectVoid &to, const xRegisterSSE &from);

extern void xMOVSSZX(const xRegisterSSE &to, const xIndirectVoid &from);
extern void xMOVSDZX(const xRegisterSSE &to, const xIndirectVoid &from);

extern void xMOVNTDQA(const xRegisterSSE &to, const xIndirectVoid &from);
extern void xMOVNTDQA(const xIndirectVoid &to, const xRegisterSSE &from);

extern void xMOVNTPD(const xIndirectVoid &to, const xRegisterSSE &from);
extern void xMOVNTPS(const xIndirectVoid &to, const xRegisterSSE &from);

extern void xMOVMSKPS(const xRegister32 &to, const xRegisterSSE &from);
extern void xMOVMSKPD(const xRegister32 &to, const xRegisterSSE &from);

// ------------------------------------------------------------------------

extern const xImplSimd_MoveSSE xMOVAPS;
extern const xImplSimd_MoveSSE xMOVUPS;
extern const xImplSimd_MoveSSE xMOVAPD;
extern const xImplSimd_MoveSSE xMOVUPD;

#ifdef ALWAYS_USE_MOVAPS
extern const xImplSimd_MoveSSE xMOVDQA;
extern const xImplSimd_MoveSSE xMOVDQU;
#else
extern const xImplSimd_MoveDQ xMOVDQA;
extern const xImplSimd_MoveDQ xMOVDQU;
#endif

extern const xImplSimd_MovHL xMOVH;
extern const xImplSimd_MovHL xMOVL;
extern const xImplSimd_MovHL_RtoR xMOVLH;
extern const xImplSimd_MovHL_RtoR xMOVHL;

extern const xImplSimd_Blend xBLEND;
extern const xImplSimd_PMove xPMOVSX;
extern const xImplSimd_PMove xPMOVZX;

extern const xImplSimd_DestRegSSE xMOVSLDUP;
extern const xImplSimd_DestRegSSE xMOVSHDUP;

extern void xINSERTPS(const xRegisterSSE &to, const xRegisterSSE &from, u8 imm8);
extern void xINSERTPS(const xRegisterSSE &to, const xIndirect32 &from, u8 imm8);

extern void xEXTRACTPS(const xRegister32or64 &to, const xRegisterSSE &from, u8 imm8);
extern void xEXTRACTPS(const xIndirect32 &dest, const xRegisterSSE &from, u8 imm8);

// ------------------------------------------------------------------------

extern const xImplSimd_DestRegEither xPAND;
extern const xImplSimd_DestRegEither xPANDN;
extern const xImplSimd_DestRegEither xPOR;
extern const xImplSimd_DestRegEither xPXOR;

extern const xImplSimd_Shuffle xSHUF;

// ------------------------------------------------------------------------

extern const xImplSimd_DestRegSSE xPTEST;

extern const xImplSimd_MinMax xMIN;
extern const xImplSimd_MinMax xMAX;

extern const xImplSimd_Compare xCMPEQ, xCMPLT,
    xCMPLE, xCMPUNORD,
    xCMPNE, xCMPNLT,
    xCMPNLE, xCMPORD;

extern const xImplSimd_COMI xCOMI;
extern const xImplSimd_COMI xUCOMI;

extern const xImplSimd_PCompare xPCMP;
extern const xImplSimd_PMinMax xPMIN;
extern const xImplSimd_PMinMax xPMAX;

// ------------------------------------------------------------------------
//
//
extern void xCVTDQ2PD(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTDQ2PD(const xRegisterSSE &to, const xIndirect64 &from);
extern void xCVTDQ2PS(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTDQ2PS(const xRegisterSSE &to, const xIndirect128 &from);

extern void xCVTPD2DQ(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTPD2DQ(const xRegisterSSE &to, const xIndirect128 &from);
extern void xCVTPD2PS(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTPD2PS(const xRegisterSSE &to, const xIndirect128 &from);

extern void xCVTPI2PD(const xRegisterSSE &to, const xIndirect64 &from);
extern void xCVTPI2PS(const xRegisterSSE &to, const xIndirect64 &from);

extern void xCVTPS2DQ(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTPS2DQ(const xRegisterSSE &to, const xIndirect128 &from);
extern void xCVTPS2PD(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTPS2PD(const xRegisterSSE &to, const xIndirect64 &from);

extern void xCVTSD2SI(const xRegister32or64 &to, const xRegisterSSE &from);
extern void xCVTSD2SI(const xRegister32or64 &to, const xIndirect64 &from);
extern void xCVTSD2SS(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTSD2SS(const xRegisterSSE &to, const xIndirect64 &from);
extern void xCVTSI2SS(const xRegisterSSE &to, const xRegister32or64 &from);
extern void xCVTSI2SS(const xRegisterSSE &to, const xIndirect32 &from);

extern void xCVTSS2SD(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTSS2SD(const xRegisterSSE &to, const xIndirect32 &from);
extern void xCVTSS2SI(const xRegister32or64 &to, const xRegisterSSE &from);
extern void xCVTSS2SI(const xRegister32or64 &to, const xIndirect32 &from);

extern void xCVTTPD2DQ(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTTPD2DQ(const xRegisterSSE &to, const xIndirect128 &from);
extern void xCVTTPS2DQ(const xRegisterSSE &to, const xRegisterSSE &from);
extern void xCVTTPS2DQ(const xRegisterSSE &to, const xIndirect128 &from);

extern void xCVTTSD2SI(const xRegister32or64 &to, const xRegisterSSE &from);
extern void xCVTTSD2SI(const xRegister32or64 &to, const xIndirect64 &from);
extern void xCVTTSS2SI(const xRegister32or64 &to, const xRegisterSSE &from);
extern void xCVTTSS2SI(const xRegister32or64 &to, const xIndirect32 &from);

// ------------------------------------------------------------------------

extern const xImplSimd_AndNot xANDN;
extern const xImplSimd_rSqrt xRCP;
extern const xImplSimd_rSqrt xRSQRT;
extern const xImplSimd_Sqrt xSQRT;

extern const xImplSimd_Shift xPSLL;
extern const xImplSimd_Shift xPSRL;
extern const xImplSimd_ShiftWithoutQ xPSRA;
extern const xImplSimd_AddSub xPADD;
extern const xImplSimd_AddSub xPSUB;
extern const xImplSimd_PMul xPMUL;
extern const xImplSimd_PAbsolute xPABS;
extern const xImplSimd_PSign xPSIGN;
extern const xImplSimd_PMultAdd xPMADD;
extern const xImplSimd_HorizAdd xHADD;
extern const xImplSimd_DotProduct xDP;
extern const xImplSimd_Round xROUND;

extern const xImplSimd_PShuffle xPSHUF;
extern const SimdImpl_PUnpack xPUNPCK;
extern const xImplSimd_Unpack xUNPCK;
extern const SimdImpl_Pack xPACK;
}

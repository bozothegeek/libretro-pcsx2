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

#pragma once

#include "Pcsx2Types.h"

#ifdef __M_X86_64
static const uint iREGCNT_XMM = 16;
static const uint iREGCNT_GPR = 16;
#else
// Register counts for x86/32 mode:
static const uint iREGCNT_XMM = 8;
static const uint iREGCNT_GPR = 8;
#endif

enum XMMSSEType {
    XMMT_INT = 0, // integer (sse2 only)
    XMMT_FPS = 1 // floating point
};

// --------------------------------------------------------------------------------------
//  __tls_emit / x86EMIT_MULTITHREADED
// --------------------------------------------------------------------------------------
// Multithreaded support for the x86 emitter.  (defaults to 0)
// To enable the multithreaded emitter, either set the below define to 1, or set the define
// as a project option.  The multithreaded emitter relies on native compiler support for
// TLS -- Macs are crap out of luck there (for now).
#include "Utilities/Dependencies.h"
#include "Utilities/Threading.h"

#ifndef x86EMIT_MULTITHREADED
#if PCSX2_THREAD_LOCAL
#define x86EMIT_MULTITHREADED 1
#else
// No TLS support?  Force-clear the MT flag:
#pragma message("x86emitter: TLS not available, multithreaded emitter disabled.")
#undef x86EMIT_MULTITHREADED
#define x86EMIT_MULTITHREADED 0
#endif
#endif

#ifndef __tls_emit
#if x86EMIT_MULTITHREADED
#define __tls_emit thread_local
#else
// Using TlsVariable is sub-optimal and could result in huge executables, so we
// force-disable TLS entirely, and disallow running multithreaded recompilation
// components within PCSX2 manually.
#define __tls_emit
#endif
#endif

extern __tls_emit u8 *x86Ptr;
extern __tls_emit XMMSSEType g_xmmtypes[iREGCNT_XMM];

namespace x86Emitter
{

extern void xWrite8(u8 val);
extern void xWrite16(u16 val);
extern void xWrite32(u32 val);
extern void xWrite64(u64 val);

//------------------------------------------------------------------
// templated version of is_s8 is required, so that u16's get correct sign extension treatment.
template <typename T>
static __forceinline bool is_s8(T imm)
{
    return (s8)imm == (typename std::make_signed<T>::type)imm;
}

template <typename T>
void xWrite(T val);

// --------------------------------------------------------------------------------------
//  ALWAYS_USE_MOVAPS [define] / AlwaysUseMovaps [const]
// --------------------------------------------------------------------------------------
// This tells the recompiler's emitter to always use movaps instead of movdqa.  Both instructions
// do the exact same thing, but movaps is 1 byte shorter, and thus results in a cleaner L1 cache
// and some marginal speed gains as a result.  (it's possible someday in the future the per-
// formance of the two instructions could change, so this constant is provided to restore MOVDQA
// use easily at a later time, if needed).
//
#define ALWAYS_USE_MOVAPS

// --------------------------------------------------------------------------------------
//  __emitline - preprocessors definition
// --------------------------------------------------------------------------------------
// This is configured to inline emitter functions appropriately for release builds, and
// disables some of the more aggressive inlines for dev builds (which can be helpful when
// debugging).  Additionally,  I've set up the inlining to be as practical and intelligent
// as possible with regard to constant propagation.  Namely this involves forcing inlining
// for (void*) forms of ModRM, which (thanks to constprop) reduce to virtually no code, and
// force-disabling inlining on complicated SibSB forms [since MSVC would sometimes inline
// despite being a generally bad idea].
//
// In the case of (Reg, Imm) forms, the inlining is up to the discreation of the compiler.
//
// Note: I *intentionally* use __forceinline directly for most single-line class members,
// when needed.  There's no point in using __emitline in these cases since the debugger
// can't trace into single-line functions anyway.
//
#define __emitinline __forceinline

// ModRM 'mod' field enumeration.   Provided mostly for reference:
enum ModRm_ModField {
    Mod_NoDisp = 0, // effective address operation with no displacement, in the form of [reg] (or uses special Disp32-only encoding in the case of [ebp] form)
    Mod_Disp8,      // effective address operation with 8 bit displacement, in the form of [reg+disp8]
    Mod_Disp32,     // effective address operation with 32 bit displacement, in the form of [reg+disp32],
    Mod_Direct      // direct reg/reg operation
};

// ----------------------------------------------------------------------------
// JccComparisonType - enumerated possibilities for inspired code branching!
//
enum JccComparisonType {
    Jcc_Unknown = -2,
    Jcc_Unconditional = -1,
    Jcc_Overflow = 0x0,
    Jcc_NotOverflow = 0x1,
    Jcc_Below = 0x2,
    Jcc_Carry = 0x2,
    Jcc_AboveOrEqual = 0x3,
    Jcc_NotCarry = 0x3,
    Jcc_Zero = 0x4,
    Jcc_Equal = 0x4,
    Jcc_NotZero = 0x5,
    Jcc_NotEqual = 0x5,
    Jcc_BelowOrEqual = 0x6,
    Jcc_Above = 0x7,
    Jcc_Signed = 0x8,
    Jcc_Unsigned = 0x9,
    Jcc_ParityEven = 0xa,
    Jcc_ParityOdd = 0xb,
    Jcc_Less = 0xc,
    Jcc_GreaterOrEqual = 0xd,
    Jcc_LessOrEqual = 0xe,
    Jcc_Greater = 0xf
};

// Not supported yet:
//E3 cb 	JECXZ rel8 	Jump short if ECX register is 0.

// ----------------------------------------------------------------------------
// SSE2_ComparisonType - enumerated possibilities for SIMD data comparison!
//
enum SSE2_ComparisonType {
    SSE2_Equal = 0,
    SSE2_Less,
    SSE2_LessOrEqual,
    SSE2_Unordered,
    SSE2_NotEqual,
    SSE2_NotLess,
    SSE2_NotLessOrEqual,
    SSE2_Ordered
};

static const int ModRm_UseSib = 4;    // same index value as ESP (used in RM field)
static const int ModRm_UseDisp32 = 5; // same index value as EBP (used in Mod field)
static const int Sib_EIZ = 4;         // same index value as ESP (used in Index field)
static const int Sib_UseDisp32 = 5;   // same index value as EBP (used in Base field)
				      //
// Assigns the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
#define xSetPtr(ptr) (x86Ptr = (u8*)(ptr))

#define xAdvancePtr(bytes) ((x86Ptr += (bytes)))

// Retrieves the current emitter buffer target address.
// This is provided instead of using x86Ptr directly, since we may in the future find
// a need to change the storage class system for the x86Ptr 'under the hood.'
#define xGetPtr() (x86Ptr)

extern u8 *xGetAlignedCallTarget(void);

extern JccComparisonType xInvertCond(JccComparisonType src);

class xAddressVoid;

// --------------------------------------------------------------------------------------
//  OperandSizedObject
// --------------------------------------------------------------------------------------
class OperandSizedObject
{
protected:
    uint _operandSize = 0;
    OperandSizedObject() = default;
    OperandSizedObject(uint operandSize)
        : _operandSize(operandSize)
    {
    }
public:
    uint GetOperandSize() const {
        return _operandSize;
    }

    bool Is8BitOp() const { return _operandSize == 1; }
    u8 GetPrefix16() const { return _operandSize == 2 ? 0x66 : 0; }
    void prefix16() const
    {
        if (_operandSize == 2)
            xWrite8(0x66);
    }

    int GetImmSize() const {
        switch (_operandSize) {
            case 1: return 1;
            case 2: return 2;
            case 4: return 4;
            case 8: return 4; // Only mov's take 64-bit immediates
	    default:
		    break;
        }
        return 0;
    }

    void xWriteImm(int imm) const
    {
        switch (GetImmSize()) {
            case 1:
                xWrite8(imm);
                break;
            case 2:
                xWrite16(imm);
                break;
            case 4:
                xWrite32(imm);
                break;
	    default:
		break;
        }
    }
};

// Represents an unused or "empty" register assignment.  If encountered by the emitter, this
// will be ignored (in some cases it is disallowed and generates an assertion)
static const int xRegId_Empty = -1;

// Represents an invalid or uninitialized register.  If this is encountered by the emitter it
// will generate an assertion.
static const int xRegId_Invalid = -2;

// --------------------------------------------------------------------------------------
//  xRegisterBase  -  type-unsafe x86 register representation.
// --------------------------------------------------------------------------------------
// Unless doing some fundamental stuff, use the friendly xRegister32/16/8 and xRegisterSSE
// instead, which are built using this class and provide strict register type safety when
// passed into emitter instructions.
//
class xRegisterBase : public OperandSizedObject
{
protected:
    xRegisterBase(uint operandSize, int regId)
        : OperandSizedObject(operandSize), Id(regId)
    {
        // Note: to avoid tons of ifdef, the 32 bits build will instantiate
        // all 16x64 bits registers.
    }
public:
    int Id;

    xRegisterBase()
        : OperandSizedObject(0), Id(xRegId_Invalid)
    {
    }

    bool IsEmpty() const { return Id < 0; }
    bool IsInvalid() const { return Id == xRegId_Invalid; }
    bool IsExtended() const { return Id > 7; } // Register 8-15 need an extra bit to be selected
    bool IsMem() const { return false; }
    bool IsReg() const { return true; }

    // Returns true if the register is a valid accumulator: Eax, Ax, Al, XMM0.
    bool IsAccumulator() const { return Id == 0; }

// IsWide: return true if the register is 64 bits (requires a wide op on the rex prefix)
#ifdef __M_X86_64
    bool IsWide() const
    {
        return _operandSize == 8;
    }
#else
    bool IsWide() const
    {
        return false;
    } // no 64 bits GPR
#endif
    // return true if the register is a valid YMM register
    bool IsWideSIMD() const { return _operandSize == 32; }

    // Diagnostics -- returns a string representation of this register.  Return string
    // is a valid non-null string for any Id, valid or invalid.  No assertions are generated.
    int GetId() const { return Id; }
};

class xRegisterInt : public xRegisterBase
{
    typedef xRegisterBase _parent;

protected:
    explicit xRegisterInt(uint operandSize, int regId)
        : _parent(operandSize, regId)
    {
    }
public:
    xRegisterInt() = default;

    /// Checks if mapping the ID directly would be a good idea
    bool canMapIDTo(int otherSize) const
    {
	if ((otherSize == 1) == (_operandSize == 1))
            return true;
	/// IDs in [4, 8) are h registers in 8-bit
        return Id < 4 || Id >= 8;
    }

    /// Get a non-wide version of the register (for use with e.g. mov, where `mov eax, 3` and `mov rax, 3` are functionally identical but `mov eax, 3` is shorter)
    xRegisterInt GetNonWide(void) const
    {
        return _operandSize == 8 ? xRegisterInt(4, Id) : *this;
    }

    xRegisterInt MatchSizeTo(xRegisterInt other) const;

    bool operator==(const xRegisterInt &src) const { return Id == src.Id && (_operandSize == src._operandSize); }
    bool operator!=(const xRegisterInt &src) const { return !operator==(src); }
};

// --------------------------------------------------------------------------------------
//  xRegister8/16/32/64  -  Represents a basic 8/16/32/64 bit GPR on the x86
// --------------------------------------------------------------------------------------
class xRegister8 : public xRegisterInt
{
    typedef xRegisterInt _parent;

public:
    xRegister8() = default;
    explicit xRegister8(int regId)
        : _parent(1, regId)
    {
    }
    explicit xRegister8(const xRegisterInt& other)
        : _parent(1, other.Id)
    {
    }

    bool operator==(const xRegister8 &src) const { return Id == src.Id; }
    bool operator!=(const xRegister8 &src) const { return Id != src.Id; }
};

class xRegister16 : public xRegisterInt
{
    typedef xRegisterInt _parent;

public:
    xRegister16() = default;
    explicit xRegister16(int regId)
        : _parent(2, regId)
    {
    }
    explicit xRegister16(const xRegisterInt& other)
        : _parent(2, other.Id)
    {
    }

    bool operator==(const xRegister16 &src) const { return this->Id == src.Id; }
    bool operator!=(const xRegister16 &src) const { return this->Id != src.Id; }
};

class xRegister32 : public xRegisterInt
{
    typedef xRegisterInt _parent;

public:
    xRegister32() = default;
    explicit xRegister32(int regId)
        : _parent(4, regId)
    {
    }
    explicit xRegister32(const xRegisterInt& other)
        : _parent(4, other.Id)
    {
    }

    bool operator==(const xRegister32 &src) const { return this->Id == src.Id; }
    bool operator!=(const xRegister32 &src) const { return this->Id != src.Id; }
};

class xRegister64 : public xRegisterInt
{
    typedef xRegisterInt _parent;

public:
    xRegister64() = default;
    explicit xRegister64(int regId)
        : _parent(8, regId)
    {
    }
    explicit xRegister64(const xRegisterInt& other)
        : _parent(8, other.Id)
    {
    }

    bool operator==(const xRegister64 &src) const { return this->Id == src.Id; }
    bool operator!=(const xRegister64 &src) const { return this->Id != src.Id; }
};

// --------------------------------------------------------------------------------------
//  xRegisterSSE  -  Represents either a 64 bit or 128 bit SIMD register
// --------------------------------------------------------------------------------------
// This register type is provided to allow legal syntax for instructions that accept
// an XMM register as a parameter, but do not allow for a GPR.

class xRegisterSSE : public xRegisterBase
{
    typedef xRegisterBase _parent;

public:
    xRegisterSSE() = default;
    explicit xRegisterSSE(int regId)
        : _parent(16, regId)
    {
    }

    bool operator==(const xRegisterSSE &src) const { return this->Id == src.Id; }
    bool operator!=(const xRegisterSSE &src) const { return this->Id != src.Id; }

    static const inline xRegisterSSE &GetInstance(uint id);
};

class xRegisterCL : public xRegister8
{
public:
    xRegisterCL()
        : xRegister8(1)
    {
    }
};

// --------------------------------------------------------------------------------------
//  xAddressReg
// --------------------------------------------------------------------------------------
// Use 32/64 bit registers as our index registers (for ModSib-style memory address calculations).
// This type is implicitly exchangeable with xRegister32/64.
//
// Only xAddressReg provides operators for constructing xAddressInfo types.  These operators
// could have been added to xRegister32/64 directly instead, however I think this design makes
// more sense and allows the programmer a little more type protection if needed.
//

#ifdef __M_X86_64
#define xRegisterLong xRegister64
#else
#define xRegisterLong xRegister32
#endif
static const int wordsize = sizeof(sptr);

class xAddressReg : public xRegisterLong
{
public:
    xAddressReg() = default;
    explicit xAddressReg(xRegisterInt other)
        : xRegisterLong(other)
    {
    }
    explicit xAddressReg(int regId)
        : xRegisterLong(regId)
    {
    }

    xAddressVoid operator+(const xAddressReg &right) const;
    xAddressVoid operator+(sptr right) const;
    xAddressVoid operator+(const void *right) const;
    xAddressVoid operator-(sptr right) const;
    xAddressVoid operator-(const void *right) const;
    xAddressVoid operator*(int factor) const;
    xAddressVoid operator<<(u32 shift) const;
    xAddressReg& operator=(const xAddressReg&) = default;
};

// --------------------------------------------------------------------------------------
//  xRegisterEmpty
// --------------------------------------------------------------------------------------
struct xRegisterEmpty
{
    operator xRegister8() const
    {
        return xRegister8(xRegId_Empty);
    }

    operator xRegister16() const
    {
        return xRegister16(xRegId_Empty);
    }

    operator xRegister32() const
    {
        return xRegister32(xRegId_Empty);
    }

    operator xRegisterSSE() const
    {
        return xRegisterSSE(xRegId_Empty);
    }

    operator xAddressReg() const
    {
        return xAddressReg(xRegId_Empty);
    }
};

// FIXME This one is likely useless and superseeded by the future xRegister16or32or64
class xRegister16or32
{
protected:
    const xRegisterInt &m_convtype;

public:
    xRegister16or32(const xRegister32 &src)
        : m_convtype(src)
    {
    }
    xRegister16or32(const xRegister16 &src)
        : m_convtype(src)
    {
    }

    operator const xRegisterBase &() const { return m_convtype; }

    const xRegisterInt *operator->() const
    {
        return &m_convtype;
    }
};

class xRegister16or32or64
{
protected:
    const xRegisterInt &m_convtype;

public:
    xRegister16or32or64(const xRegister64 &src)
        : m_convtype(src)
    {
    }
    xRegister16or32or64(const xRegister32 &src)
        : m_convtype(src)
    {
    }
    xRegister16or32or64(const xRegister16 &src)
        : m_convtype(src)
    {
    }

    operator const xRegisterBase &() const { return m_convtype; }

    const xRegisterInt *operator->() const
    {
        return &m_convtype;
    }
};

class xRegister32or64
{
protected:
    const xRegisterInt &m_convtype;

public:
    xRegister32or64(const xRegister64 &src)
        : m_convtype(src)
    {
    }
    xRegister32or64(const xRegister32 &src)
        : m_convtype(src)
    {
    }

    operator const xRegisterBase &() const { return m_convtype; }

    const xRegisterInt *operator->() const
    {
        return &m_convtype;
    }
};

extern const xRegisterEmpty xEmptyReg;

// clang-format off

extern const xRegisterSSE
    xmm0, xmm1, xmm2, xmm3,
    xmm4, xmm5, xmm6, xmm7,
    xmm8, xmm9, xmm10, xmm11,
    xmm12, xmm13, xmm14, xmm15;

extern const xAddressReg
    rax, rbx, rcx, rdx,
    rsi, rdi, rbp, rsp,
    r8, r9, r10, r11,
    r12, r13, r14, r15;

extern const xRegister32
     eax,  ebx,  ecx,  edx,
     esi,  edi,  ebp,  esp,
     r8d,  r9d, r10d, r11d,
    r12d, r13d, r14d, r15d;

extern const xRegister16
    ax, bx, cx, dx,
    si, di, bp, sp;

extern const xRegister8
    al, dl, bl,
    ah, ch, dh, bh;

extern const xAddressReg
    arg1reg, arg2reg,
    arg3reg, arg4reg,
    calleeSavedReg1,
    calleeSavedReg2;


extern const xRegister32
    arg1regd, arg2regd,
    calleeSavedReg1d,
    calleeSavedReg2d;


// clang-format on

extern const xRegisterCL cl; // I'm special!

const xRegisterSSE &xRegisterSSE::GetInstance(uint id)
{
    static const xRegisterSSE *const m_tbl_xmmRegs[] =
        {
            &xmm0, &xmm1, &xmm2, &xmm3,
            &xmm4, &xmm5, &xmm6, &xmm7,
            &xmm8, &xmm9, &xmm10, &xmm11,
            &xmm12, &xmm13, &xmm14, &xmm15};

    return *m_tbl_xmmRegs[id];
}

// --------------------------------------------------------------------------------------
//  xAddressVoid
// --------------------------------------------------------------------------------------
class xAddressVoid
{
public:
    xAddressReg Base;  // base register (no scale)
    xAddressReg Index; // index reg gets multiplied by the scale
    int Factor;        // scale applied to the index register, in factor form (not a shift!)
    sptr Displacement;  // address displacement // 4B max even on 64 bits but keep rest for assertions

public:
    xAddressVoid(const xAddressReg &base, const xAddressReg &index, int factor = 1, sptr displacement = 0);

    xAddressVoid(const xAddressReg &index, sptr displacement = 0);
    explicit xAddressVoid(const void *displacement);
    explicit xAddressVoid(sptr displacement = 0);

public:
    bool IsByteSizeDisp() const { return is_s8(Displacement); }

    xAddressVoid &Add(sptr imm)
    {
        Displacement += imm;
        return *this;
    }

    xAddressVoid &Add(const xAddressReg &src);
    xAddressVoid &Add(const xAddressVoid &src);

    __forceinline xAddressVoid operator+(const xAddressReg &right) const { return xAddressVoid(*this).Add(right); }
    __forceinline xAddressVoid operator+(const xAddressVoid &right) const { return xAddressVoid(*this).Add(right); }
    __forceinline xAddressVoid operator+(sptr imm) const { return xAddressVoid(*this).Add(imm); }
    __forceinline xAddressVoid operator-(sptr imm) const { return xAddressVoid(*this).Add(-imm); }
    __forceinline xAddressVoid operator+(const void *addr) const { return xAddressVoid(*this).Add((uptr)addr); }

    __forceinline void operator+=(const xAddressReg &right) { Add(right); }
    __forceinline void operator+=(sptr imm) { Add(imm); }
    __forceinline void operator-=(sptr imm) { Add(-imm); }
};

// --------------------------------------------------------------------------------------
//  xAddressInfo
// --------------------------------------------------------------------------------------
template <typename BaseType>
class xAddressInfo : public xAddressVoid
{
    typedef xAddressVoid _parent;

public:
    xAddressInfo(const xAddressReg &base, const xAddressReg &index, int factor = 1, sptr displacement = 0)
        : _parent(base, index, factor, displacement)
    {
    }

    /*xAddressInfo( const xAddressVoid& src )
			: _parent( src ) {}*/

    explicit xAddressInfo(const xAddressReg &index, sptr displacement = 0)
        : _parent(index, displacement)
    {
    }

    explicit xAddressInfo(sptr displacement = 0)
        : _parent(displacement)
    {
    }

    static xAddressInfo<BaseType> FromIndexReg(const xAddressReg &index, int scale = 0, sptr displacement = 0);

public:
    using _parent::operator+=;
    using _parent::operator-=;

    bool IsByteSizeDisp() const { return is_s8(Displacement); }

    xAddressInfo<BaseType> &Add(sptr imm)
    {
        Displacement += imm;
        return *this;
    }

    xAddressInfo<BaseType> &Add(const xAddressReg &src)
    {
        _parent::Add(src);
        return *this;
    }
    xAddressInfo<BaseType> &Add(const xAddressInfo<BaseType> &src)
    {
        _parent::Add(src);
        return *this;
    }

    __forceinline xAddressInfo<BaseType> operator+(const xAddressReg &right) const { return xAddressInfo(*this).Add(right); }
    __forceinline xAddressInfo<BaseType> operator+(const xAddressInfo<BaseType> &right) const { return xAddressInfo(*this).Add(right); }
    __forceinline xAddressInfo<BaseType> operator+(sptr imm) const { return xAddressInfo(*this).Add(imm); }
    __forceinline xAddressInfo<BaseType> operator-(sptr imm) const { return xAddressInfo(*this).Add(-imm); }
    __forceinline xAddressInfo<BaseType> operator+(const void *addr) const { return xAddressInfo(*this).Add((uptr)addr); }

    __forceinline void operator+=(const xAddressInfo<BaseType> &right) { Add(right); }
};

typedef xAddressInfo<u128> xAddress128;
typedef xAddressInfo<u64> xAddress64;
typedef xAddressInfo<u32> xAddress32;
typedef xAddressInfo<u16> xAddress16;
typedef xAddressInfo<u8> xAddress8;

static __forceinline xAddressVoid operator+(const void *addr, const xAddressVoid &right)
{
    return right + addr;
}

static __forceinline xAddressVoid operator+(sptr addr, const xAddressVoid &right)
{
    return right + addr;
}

template <typename OperandType>
static __forceinline xAddressInfo<OperandType> operator+(const void *addr, const xAddressInfo<OperandType> &right)
{
    return right + addr;
}

template <typename OperandType>
static __forceinline xAddressInfo<OperandType> operator+(sptr addr, const xAddressInfo<OperandType> &right)
{
    return right + addr;
}

// --------------------------------------------------------------------------------------
//  xImmReg< typename xRegType >
// --------------------------------------------------------------------------------------
// Used to represent an immediate value which can also be optimized to a register. Note
// that the immediate value represented by this structure is *always* legal.  The register
// assignment is an optional optimization which can be implemented in cases where an
// immediate is used enough times to merit allocating it to a register.
//
// Note: not all instructions support this operand type (yet).  You can always implement it
// manually by checking the status of IsReg() and generating the xOP conditionally.
//
template <typename xRegType>
class xImmReg
{
    xRegType m_reg;
    int m_imm;

public:
    xImmReg()
        : m_reg()
    {
        m_imm = 0;
    }

    xImmReg(int imm, const xRegType &reg = xEmptyReg)
    {
        m_reg = reg;
        m_imm = imm;
    }

    bool IsReg() const { return !m_reg.IsEmpty(); }
};

// --------------------------------------------------------------------------------------
//  xIndirectVoid - Internal low-level representation of the ModRM/SIB information.
// --------------------------------------------------------------------------------------
// This class serves two purposes:  It houses 'reduced' ModRM/SIB info only, which means
// that the Base, Index, Scale, and Displacement values are all in the correct arrange-
// ments, and it serves as a type-safe layer between the xRegister's operators (which
// generate xAddressInfo types) and the emitter's ModSib instruction forms.  Without this,
// the xRegister would pass as a ModSib type implicitly, and that would cause ambiguity
// on a number of instructions.
//
// End users should always use xAddressInfo instead.
//
class xIndirectVoid : public OperandSizedObject
{
public:
    xAddressReg Base;  // base register (no scale)
    xAddressReg Index; // index reg gets multiplied by the scale
    uint Scale;        // scale applied to the index register, in scale/shift form
    sptr Displacement; // offset applied to the Base/Index registers.
                       // Displacement is 8/32 bits even on x86_64
                       // However we need the whole pointer to calculate rip-relative offsets

public:
    explicit xIndirectVoid(sptr disp);
    explicit xIndirectVoid(const xAddressVoid &src);
    xIndirectVoid(xAddressReg base, xAddressReg index, int scale = 0, sptr displacement = 0);
    xIndirectVoid &Add(sptr imm);

    bool IsByteSizeDisp() const { return is_s8(Displacement); }
    bool IsMem() const { return true; }
    bool IsReg() const { return false; }
    bool IsExtended() const { return false; } // Non sense but ease template
    bool IsWide() const { return _operandSize == 8; }

    operator xAddressVoid()
    {
        return xAddressVoid(Base, Index, Scale, Displacement);
    }

    __forceinline xIndirectVoid operator+(const sptr imm) const { return xIndirectVoid(*this).Add(imm); }
    __forceinline xIndirectVoid operator-(const sptr imm) const { return xIndirectVoid(*this).Add(-imm); }

protected:
    void Reduce();
};

template <typename OperandType>
class xIndirect : public xIndirectVoid
{
    typedef xIndirectVoid _parent;

public:
    explicit xIndirect(sptr disp)
        : _parent(disp)
    {
        _operandSize = sizeof(OperandType);
    }
    explicit xIndirect(const xAddressInfo<OperandType> &src)
        : _parent(src)
    {
        _operandSize = sizeof(OperandType);
    }
    xIndirect(xAddressReg base, xAddressReg index, int scale = 0, sptr displacement = 0)
        : _parent(base, index, scale, displacement)
    {
        _operandSize = sizeof(OperandType);
    }
    explicit xIndirect(const xIndirectVoid& other)
        : _parent(other)
    {
    }

    xIndirect<OperandType> &Add(sptr imm)
    {
        Displacement += imm;
        return *this;
    }

    __forceinline xIndirect<OperandType> operator+(const sptr imm) const { return xIndirect(*this).Add(imm); }
    __forceinline xIndirect<OperandType> operator-(const sptr imm) const { return xIndirect(*this).Add(-imm); }

    bool operator==(const xIndirect<OperandType> &src) const
    {
        return (Base == src.Base) && (Index == src.Index) &&
               (Scale == src.Scale) && (Displacement == src.Displacement);
    }

    bool operator!=(const xIndirect<OperandType> &src) const
    {
        return !operator==(src);
    }

protected:
    void Reduce();
};

typedef xIndirect<u128> xIndirect128;
typedef xIndirect<u64> xIndirect64;
typedef xIndirect<u32> xIndirect32;
typedef xIndirect<u16> xIndirect16;
typedef xIndirect<u8> xIndirect8;
#ifdef __M_X86_64
typedef xIndirect<u64> xIndirectNative;
#else
typedef xIndirect<u32> xIndirectNative;
#endif

// --------------------------------------------------------------------------------------
//  xIndirect64orLess  -  base class 64, 32, 16, and 8 bit operand types
// --------------------------------------------------------------------------------------
class xIndirect64orLess : public xIndirectVoid
{
    typedef xIndirectVoid _parent;

public:
    xIndirect64orLess(const xIndirect8 &src)
        : _parent(src)
    {
    }
    xIndirect64orLess(const xIndirect16 &src)
        : _parent(src)
    {
    }
    xIndirect64orLess(const xIndirect32 &src)
        : _parent(src)
    {
    }
    xIndirect64orLess(const xIndirect64 &src)
        : _parent(src)
    {
    }
};

// --------------------------------------------------------------------------------------
//  xAddressIndexer
// --------------------------------------------------------------------------------------
// This is a type-translation "interface class" which provisions our ptr[] syntax.
// xAddressReg types go in, and xIndirectVoid derived types come out.
//
template <typename xModSibType>
class xAddressIndexer
{
public:
    // passthrough instruction, allows ModSib to pass silently through ptr translation
    // without doing anything and without compiler error.
    const xModSibType &operator[](const xModSibType &src) const { return src; }

    xModSibType operator[](const xAddressReg &src) const
    {
        return xModSibType(src, xEmptyReg);
    }

    xModSibType operator[](const xAddressVoid &src) const
    {
        return xModSibType(src.Base, src.Index, src.Factor, src.Displacement);
    }

    xModSibType operator[](const void *src) const
    {
        return xModSibType((uptr)src);
    }
};

// ptr[] - use this form for instructions which can resolve the address operand size from
// the other register operand sizes.
extern const xAddressIndexer<xIndirectVoid> ptr;
extern const xAddressIndexer<xIndirectNative> ptrNative;
extern const xAddressIndexer<xIndirect128> ptr128;
extern const xAddressIndexer<xIndirect64> ptr64;
extern const xAddressIndexer<xIndirect32> ptr32;
extern const xAddressIndexer<xIndirect16> ptr16;
extern const xAddressIndexer<xIndirect8> ptr8;

// --------------------------------------------------------------------------------------
//  xSmartJump
// --------------------------------------------------------------------------------------
// This class provides an interface for generating forward-based j8's or j32's "smartly"
// as per the measured displacement distance.  If the displacement is a valid s8, then
// a j8 is inserted, else a j32.
//
// Note: This class is inherently unsafe, and so it's recommended to use xForwardJump8/32
// whenever it is known that the jump destination is (or is not) short.  Only use
// xSmartJump in cases where it's unknown what jump encoding will be ideal.
//
// Important: Use this tool with caution!  xSmartJump cannot be used in cases where jump
// targets overlap, since the writeback of the second target will alter the position of
// the first target (which breaks the relative addressing).  To assist in avoiding such
// errors, xSmartJump works based on C++ block scope, where the destruction of the
// xSmartJump object (invoked by a '}') signals the target of the jump.  Example:
//
// {
//     iCMP( EAX, ECX );
//     xSmartJump jumpTo( Jcc_Above );
//     [... conditional code ...]
// }  // smartjump targets this spot.
//
// No code inside the scope can attempt to jump outside the scoped block (unless the jump
// uses an immediate addressing method, such as Register or Mod/RM forms of JMP/CALL).
// Multiple SmartJumps can be safely nested inside scopes, as long as they are properly
// scoped themselves.
//
// Performance Analysis:  j8's use 4 less byes per opcode, and thus can provide minor
// speed benefits in the form of L1/L2 cache clutter, on any CPU.  They're also notably
// faster on P4's, and mildly faster on AMDs.  (Core2's and i7's don't care)
//
class xSmartJump
{
    DeclareNoncopyableObject(xSmartJump);

protected:
    u8 *m_baseptr;          // base address of the instruction (passed to the instruction emitter)
    JccComparisonType m_cc; // comparison type of the instruction

public:
    int GetMaxInstructionSize() const
    {
        return (m_cc == Jcc_Unconditional) ? 5 : 6;
    }

    JccComparisonType GetCondition() const { return m_cc; }
    virtual ~xSmartJump();

    // ------------------------------------------------------------------------
    // ccType - Comparison type to be written back to the jump instruction position.
    //
    xSmartJump(JccComparisonType ccType)
    {
        m_baseptr = xGetPtr();
        m_cc = ccType;
        xAdvancePtr(GetMaxInstructionSize());
    }

protected:
    void SetTarget();
};

// --------------------------------------------------------------------------------------
//  xForwardJump
// --------------------------------------------------------------------------------------
// Primary use of this class is through the various xForwardJA8/xForwardJLE32/etc. helpers
// defined later in this header. :)
//

class xForwardJumpBase
{
public:
    // pointer to base of the instruction *Following* the jump.  The jump address will be
    // relative to this address.
    s8 *BasePtr;

public:
    xForwardJumpBase(uint opsize, JccComparisonType cctype);

protected:
    void _setTarget(uint opsize) const;
};

template <typename OperandType>
class xForwardJump : public xForwardJumpBase
{
public:
    static const uint OperandSize = sizeof(OperandType);

    // The jump instruction is emitted at the point of object construction.  The conditional
    // type must be valid (Jcc_Unknown generates an assertion).
    xForwardJump(JccComparisonType cctype = Jcc_Unconditional)
        : xForwardJumpBase(OperandSize, cctype)
    {
    }

    // Sets the jump target by writing back the current x86Ptr to the jump instruction.
    // This method can be called multiple times, re-writing the jump instruction's target
    // in each case. (the the last call is the one that takes effect).
    void SetTarget() const
    {
        _setTarget(OperandSize);
    }
};

static __forceinline xAddressVoid operator+(const void *addr, const xAddressReg &reg)
{
    return reg + (sptr)addr;
}

static __forceinline xAddressVoid operator+(sptr addr, const xAddressReg &reg)
{
    return reg + (sptr)addr;
}
}

#include "implement/helpers.h"

#include "implement/simd_helpers.h"
#include "implement/simd_moremovs.h"
#include "implement/simd_arithmetic.h"
#include "implement/simd_comparisons.h"
#include "implement/simd_shufflepack.h"

#include "implement/group1.h"
#include "implement/group2.h"
#include "implement/group3.h"
#include "implement/movs.h"    // cmov and movsx/zx
#include "implement/dwshift.h" // doubleword shifts!
#include "implement/incdec.h"
#include "implement/test.h"
#include "implement/jmpcall.h"

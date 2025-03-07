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

using namespace x86Emitter;

typedef xRegisterSSE xmm;
typedef xRegister32 x32;

struct microVU;

//------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------

struct mVU_Globals {
	u32		absclip[4], signbit[4], minvals[4], maxvals[4];
	u32		one[4];
	u32		Pi4[4];
	u32		T1[4], T2[4], T3[4], T4[4], T5[4], T6[4], T7[4], T8[4];
	u32		S2[4], S3[4], S4[4], S5[4];
	u32		E1[4], E2[4], E3[4], E4[4], E5[4], E6[4];
	float	FTOI_4[4], FTOI_12[4], FTOI_15[4];
	float	ITOF_4[4], ITOF_12[4], ITOF_15[4];
};

#define __four(val)	{ val, val, val, val }
static const __aligned(32) mVU_Globals mVUglob = {
	__four(0x7fffffff),		  // absclip
	__four(0x80000000),		  // signbit
	__four(0xff7fffff),		  // minvals
	__four(0x7f7fffff),		  // maxvals
	__four(0x3f800000),		  // ONE!
	__four(0x3f490fdb),		  // PI4!
	__four(0x3f7ffff5),		  // T1
	__four(0xbeaaa61c),		  // T5
	__four(0x3e4c40a6),		  // T2
	__four(0xbe0e6c63),		  // T3
	__four(0x3dc577df),		  // T4
	__four(0xbd6501c4),		  // T6
	__four(0x3cb31652),		  // T7
	__four(0xbb84d7e7),		  // T8
	__four(0xbe2aaaa4),		  // S2
	__four(0x3c08873e),		  // S3
	__four(0xb94fb21f),		  // S4
	__four(0x362e9c14),		  // S5
	__four(0x3e7fffa8),		  // E1
	__four(0x3d0007f4),		  // E2
	__four(0x3b29d3ff),		  // E3
	__four(0x3933e553),		  // E4
	__four(0x36b63510),		  // E5
	__four(0x353961ac),		  // E6
	__four(16.0),			  // FTOI_4
	__four(4096.0),			  // FTOI_12
	__four(32768.0),		  // FTOI_15
	__four(0.0625f),		  // ITOF_4
	__four(0.000244140625),	  // ITOF_12
	__four(0.000030517578125) // ITOF_15
};

static const uint _Ibit_ = 1 << 31;
static const uint _Ebit_ = 1 << 30;
static const uint _Mbit_ = 1 << 29;
static const uint _Dbit_ = 1 << 28;
static const uint _Tbit_ = 1 << 27;

static const uint divI = 0x1040000;
static const uint divD = 0x2080000;

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

#define _Ft_ ((mVU.code >> 16) & 0x1F)  // The ft part of the instruction register
#define _Fs_ ((mVU.code >> 11) & 0x1F)  // The fs part of the instruction register
#define _Fd_ ((mVU.code >>  6) & 0x1F)  // The fd part of the instruction register

#define _It_ ((mVU.code >> 16) & 0xF)   // The it part of the instruction register
#define _Is_ ((mVU.code >> 11) & 0xF)   // The is part of the instruction register
#define _Id_ ((mVU.code >>  6) & 0xF)   // The id part of the instruction register

#define _X	 ((mVU.code>>24) & 0x1)
#define _Y	 ((mVU.code>>23) & 0x1)
#define _Z	 ((mVU.code>>22) & 0x1)
#define _W	 ((mVU.code>>21) & 0x1)

#define _X_Y_Z_W	(((mVU.code >> 21 ) & 0xF))
#define _XYZW_SS	(_X+_Y+_Z+_W==1)
#define _XYZW_SS2	(_XYZW_SS && (_X_Y_Z_W != 8))
#define _XYZW_PS	(_X_Y_Z_W == 0xf)
#define _XYZWss(x)	((x==8) || (x==4) || (x==2) || (x==1))

#define _bc_	 (mVU.code & 0x3)
#define _bc_x	((mVU.code & 0x3) == 0)
#define _bc_y	((mVU.code & 0x3) == 1)
#define _bc_z	((mVU.code & 0x3) == 2)
#define _bc_w	((mVU.code & 0x3) == 3)

#define _Fsf_	((mVU.code >> 21) & 0x03)
#define _Ftf_	((mVU.code >> 23) & 0x03)

#define _Imm5_	((s16) (((mVU.code & 0x400) ? 0xfff0 : 0) | ((mVU.code >> 6) & 0xf)))
#define _Imm11_	((s32)  ((mVU.code & 0x400) ? (0xfffffc00 |  (mVU.code & 0x3ff)) : (mVU.code & 0x3ff)))
#define _Imm12_	((u32)((((mVU.code >> 21) & 0x1) << 11)   |  (mVU.code & 0x7ff)))
#define _Imm15_	((u32) (((mVU.code >> 10) & 0x7800)       |  (mVU.code & 0x7ff)))
#define _Imm24_	((u32)   (mVU.code & 0xffffff))

#define isCOP2		(mVU.cop2  != 0)
#define isVU1		(mVU.index != 0)
#define isVU0		(mVU.index == 0)
#define getIndex	(isVU1 ? 1 : 0)
#define getVUmem(x)	(((isVU1) ? (x & 0x3ff) : ((x >= 0x400) ? (x & 0x43f) : (x & 0xff))) * 16)
#define offsetSS	((_X) ? (0) : ((_Y) ? (4) : ((_Z) ? 8: 12)))
#define offsetReg	((_X) ? (0) : ((_Y) ? (1) : ((_Z) ? 2:  3)))

#define xmmT1  xmm0 // Used for regAlloc
#define xmmT2  xmm1 // Used for regAlloc
#define xmmT3  xmm2 // Used for regAlloc
#define xmmT4  xmm3 // Used for regAlloc
#define xmmT5  xmm4 // Used for regAlloc
#define xmmT6  xmm5 // Used for regAlloc
#define xmmT7  xmm6 // Used for regAlloc
#define xmmPQ  xmm7 // Holds the Value and Backup Values of P and Q regs

#define gprT1  eax // eax - Temp Reg
#define gprT2  ecx // ecx - Temp Reg
#define gprT3  edx // edx - Temp Reg
#define gprT1q rax // eax - Temp Reg
#define gprT2q rcx // ecx - Temp Reg
#define gprT3q rdx // edx - Temp Reg
#define gprT1b ax  // Low 16-bit of gprT1 (eax)
#define gprT2b cx  // Low 16-bit of gprT2 (ecx)
#define gprT3b dx  // Low 16-bit of gprT3 (edx)

#ifdef __M_X86_64
#define gprF0  ebx // Status Flag 0
#define gprF1 r12d // Status Flag 1
#define gprF2 r13d // Status Flag 2
#define gprF3 r14d // Status Flag 3
#else
#define gprF0  ebx // Status Flag 0
#define gprF1  ebp // Status Flag 1
#define gprF2  esi // Status Flag 2
#define gprF3  edi // Status Flag 3
#endif

// Function Params
#define mP microVU& mVU, int recPass
#define mV microVU& mVU
#define mF int recPass
#define mX mVU, recPass

typedef void Fntype_mVUrecInst(microVU& mVU, int recPass);
typedef Fntype_mVUrecInst* Fnptr_mVUrecInst;

// Function/Template Stuff
#define  mVUx (vuIndex ? microVU1 : microVU0)
#define  mVUop(opName)	static void opName (mP)
#define _mVUt template<int vuIndex>

// Define Passes
#define pass1 if (recPass == 0) // Analyze
#define pass2 if (recPass == 1) // Recompile
#define pass3 if (recPass == 2) // Logging
#define pass4 if (recPass == 3) // Flag stuff

// Upper Opcode Cases
#define opCase1 if (opCase == 1) // Normal Opcodes
#define opCase2 if (opCase == 2) // BC Opcodes
#define opCase3 if (opCase == 3) // I  Opcodes
#define opCase4 if (opCase == 4) // Q  Opcodes

//------------------------------------------------------------------
// Define mVUquickSearch
//------------------------------------------------------------------
extern __pagealigned u8 mVUsearchXMM[PCSX2_PAGESIZE];
typedef u32 (*mVUCall)(void*, void*);
#define mVUquickSearch(dest, src, size) ((((mVUCall)((void*)mVUsearchXMM))(dest, src)) == 0xf)
#define mVUemitSearch() { mVUcustomSearch(); }
//------------------------------------------------------------------

// Misc Macros...
#define mVUcurProg   mVU.prog.cur[0]
#define mVUblocks	 mVU.prog.cur->block
#define mVUir		 mVU.prog.IRinfo
#define mVUbranch	 mVU.prog.IRinfo.branch
#define mVUcycles	 mVU.prog.IRinfo.cycles
#define mVUcount	 mVU.prog.IRinfo.count
#define mVUpBlock	 mVU.prog.IRinfo.pBlock
#define mVUblock	 mVU.prog.IRinfo.block
#define mVUregs		 mVU.prog.IRinfo.block.pState
#define mVUregsTemp	 mVU.prog.IRinfo.regsTemp
#define iPC			 mVU.prog.IRinfo.curPC
#define mVUsFlagHack mVU.prog.IRinfo.sFlagHack
#define mVUconstReg	 mVU.prog.IRinfo.constReg
#define mVUstartPC	 mVU.prog.IRinfo.startPC
#define mVUinfo		 mVU.prog.IRinfo.info[iPC / 2]
#define mVUstall	 mVUinfo.stall
#define mVUup		 mVUinfo.uOp
#define mVUlow		 mVUinfo.lOp
#define sFLAG		 mVUinfo.sFlag
#define mFLAG		 mVUinfo.mFlag
#define cFLAG		 mVUinfo.cFlag
#define mVUrange     (mVUcurProg.ranges[0])[0]
#define isEvilBlock	 (mVUpBlock->pState.blockType == 2)
#define isBadOrEvil  (mVUlow.badBranch || mVUlow.evilBranch)
#define xPC			 ((iPC / 2) * 8)
#define curI		 ((u32*)mVU.regs().Micro)[iPC] //mVUcurProg.data[iPC]
#define setCode()	 { mVU.code = curI; }
#define bSaveAddr	 (((xPC + 16) & (mVU.microMemSize-8)) / 8)
#define shufflePQ	 (((mVU.p) ? 0xb0 : 0xe0) | ((mVU.q) ? 0x01 : 0x04))
#define Rmem		 &mVU.regs().VI[REG_R].UL
#define aWrap(x, m)	 ((x > m) ? 0 : x)
#define shuffleSS(x) ((x==1)?(0x27):((x==2)?(0xc6):((x==4)?(0xe1):(0xe4))))
#define clampE       CHECK_VU_EXTRA_OVERFLOW
#define islowerOP    ((iPC & 1) == 0)

#define blockCreate(addr) {												\
	if  (!mVUblocks[addr]) mVUblocks[addr] = new microBlockManager();	\
}

// Fetches the PC and instruction opcode relative to the current PC.  Used to rewind and
// fast-forward the IR state while calculating VU pipeline conditions (branches, writebacks, etc)
#define incPC(x)	 { iPC = ((iPC + (x)) & mVU.progMemMask); mVU.code = curI; }
#define incPC2(x)	 { iPC = ((iPC + (x)) & mVU.progMemMask); }

// Flag Info (Set if next-block's first 4 ops will read current-block's flags)
#define __Status	 (mVUregs.needExactMatch & 1)
#define __Mac		 (mVUregs.needExactMatch & 2)
#define __Clip		 (mVUregs.needExactMatch & 4)

//------------------------------------------------------------------
// Optimization / Debug Options
//------------------------------------------------------------------

// Reg Alloc
static const bool doRegAlloc = true; // Set to false to flush every 32bit Instruction
// This turns off reg alloc for the most part, but reg alloc will still
// be done within instructions... Also on doSwapOp() regAlloc is needed between
// Lower and Upper instructions, so in this case it flushes after the full
// 64bit instruction (lower and upper)

// No Flag Optimizations
static const bool noFlagOpts = false; // Set to true to disable all flag setting optimizations
// Note: The flag optimizations this disables should all be harmless, so
// this option is mainly just for debugging... it effectively forces mVU
// to always update Mac and Status Flags (both sticky and non-sticky) whenever
// an Upper Instruction updates them. It also always transfers the 4 possible
// flag instances between blocks...

// Multiple Flag Instances
static const bool doSFlagInsts = true; // Set to true to enable multiple status flag instances
static const bool doMFlagInsts = true; // Set to true to enable multiple mac    flag instances
static const bool doCFlagInsts = true; // Set to true to enable multiple clip   flag instances
// This is the correct behavior of the VU's. Due to the pipeline of the VU's
// there can be up to 4 different instances of values to keep track of
// for the 3 different types of flags: Status, Mac, Clip flags.
// Setting one of these to 0 acts as if there is only 1 instance of the
// corresponding flag, which may be useful when debugging flag pipeline bugs.

// Branch in Branch Delay Slots
static const bool doBranchInDelaySlot = true; // Set to true to enable evil-branches
// This attempts to emulate the correct behavior for branches in branch delay
// slots. It is evil that games do this, and handling the different possible
// cases is tricky and bug prone. If this option is disabled then the second
// branch is treated as a NOP and effectively ignored.

// Constant Propagation
static const bool doConstProp = false; // Set to true to turn on vi15 const propagation
// Enables Constant Propagation for Jumps based on vi15 'link-register'
// allowing us to know many indirect jump target addresses.
// Makes GoW a lot slower due to extra recompilation time and extra code-gen!

// Indirect Jump Caching
static const bool doJumpCaching = true; // Set to true to enable jump caching
// Indirect jumps (JR/JALR) will remember the entry points to their previously
// jumped-to addresses. This allows us to skip the microBlockManager::search()
// routine that is performed every indirect jump in order to find a block within a
// program that matches the correct pipeline state.

// Indirect Jumps are part of same cached microProgram
static const bool doJumpAsSameProgram = false; // Set to true to treat jumps as same program
// Enabling this treats indirect jumps (JR/JALR) as part of the same microProgram
// when determining the valid ranges for the microProgram cache. Disabling this
// counts indirect jumps as separate cached microPrograms which generally leads
// to more microPrograms being cached, but the programs created are smaller and
// the overall cache usage ends up being more optimal; it can also help prevent
// constant recompilation problems in certain games.
// Note: You MUST disable doJumpCaching if you enable this option.

// Handling of D-Bit in Micro Programs
static const bool doDBitHandling = false;
// This flag shouldn't be enabled in released versions of games. Any games which
// need this method of pausing the VU should be using the T-Bit instead, however
// this could prove useful for VU debugging.

//------------------------------------------------------------------
// Speed Hacks (can cause infinite loops, SPS, Black Screens, etc...)
//------------------------------------------------------------------

// Status Flag Speed Hack
#define CHECK_VU_FLAGHACK  (EmuConfig.Speedhacks.vuFlagHack)
// This hack only updates the Status Flag on blocks that will read it.
// Most blocks do not read status flags, so this is a big speedup.

//------------------------------------------------------------------
// Unknown Data
//------------------------------------------------------------------

// XG Kick Transfer Delay Amount
#define mVU_XGKICK_CYCLES ((CHECK_XGKICKHACK) ? 6 : 1)
// Its unknown at recompile time how long the xgkick transfer will take
// so give it a value that makes games happy :) (SO3 is fine at 1 cycle delay)

//------------------------------------------------------------------

extern void mVUsaveReg_SSE4(const xmm& reg, xAddressVoid ptr, int xyzw, bool modXYZW);
extern void mVUsaveReg_C(const xmm& reg, xAddressVoid ptr, int xyzw, bool modXYZW);

extern void mVUmergeRegs(const xmm& dest, const xmm& src,  int xyzw, bool modXYZW=false);
extern void (*mVUsaveReg)(const xmm& reg, xAddressVoid ptr, int xyzw, bool modXYZW);
extern void mVUloadReg(const xmm& reg, xAddressVoid ptr, int xyzw);

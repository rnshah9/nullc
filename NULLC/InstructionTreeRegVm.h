#pragma once

#include <stdint.h>

enum RegVmInstructionCode
{
	rviNop,

	rviLoadByte,
	rviLoadWord,
	rviLoadDword,
	rviLoadQword,
	rviLoadFloat,

	rviLoadImm,
	rviLoadImmHigh,

	rviStoreByte,
	rviStoreWord,
	rviStoreDword,
	rviStoreQword,
	rviStoreFloat,

	rviCombinedd,
	rviMov,

	rviDtoi,
	rviDtol,
	rviDtof,
	rviItod,
	rviLtod,
	rviItol,
	rviLtoi,

	rviIndex,

	rviGetAddr,

	rviSetRange,

	rviJmp,
	rviJmpz,
	rviJmpnz,

	rviPush,
	rviPushq,
	rviPushImm,
	rviPushImmq,

	rviPop,
	rviPopq,

	rviCall,
	rviCallPtr,

	rviReturn,

	rviPushvtop,

	rviAdd,
	rviSub,
	rviMul,
	rviDiv,
	rviPow,
	rviMod,

	rviLess,
	rviGreater,
	rviLequal,
	rviGequal,
	rviEqual,
	rviNequal,

	rviShl,
	rviShr,
	
	rviBitAnd,
	rviBitOr,
	rviBitXor,

	rviLogXor,

	rviAddl,
	rviSubl,
	rviMull,
	rviDivl,
	rviPowl,
	rviModl,

	rviLessl,
	rviGreaterl,
	rviLequall,
	rviGequall,
	rviEquall,
	rviNequall,

	rviShll,
	rviShrl,

	rviBitAndl,
	rviBitOrl,
	rviBitXorl,

	rviLogXorl,

	rviAddd,
	rviSubd,
	rviMuld,
	rviDivd,
	rviPowd,
	rviModd,

	rviLessd,
	rviGreaterd,
	rviLequald,
	rviGequald,
	rviEquald,
	rviNequald,

	rviNeg,
	rviNegl,
	rviNegd,

	rviBitNot,
	rviBitNotl,

	rviLogNot,
	rviLogNotl,

	rviConvertPtr,

	rviCheckRet,

	// Temporary instructions, no execution
	rviFuncAddr,
	rviTypeid,
};

enum RegVmSetRangeType
{
	rvsrDouble,
	rvsrFloat,
	rvsrLong,
	rvsrInt,
	rvsrShort,
	rvsrChar,
};

enum RegVmReturnType
{
	rvrVoid,
	rvrDouble,
	rvrLong,
	rvrInt,
	rvrStruct,
	rvrError,
};

#define rvrrGlobals 0
#define rvrrFrame 1

#define rvrrCount 2

struct RegVmCmd
{
	RegVmCmd(): code(0), rA(0), rB(0), rC(0), argument(0)
	{
	}

	RegVmCmd(RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument): code((unsigned char)code), rA(rA), rB(rB), rC(rC), argument(argument)
	{
	}

	unsigned char code;
	unsigned char rA;
	unsigned char rB;
	unsigned char rC;
	unsigned argument;
};

union RegVmRegister
{
	// Debug testing only
	RegVmReturnType activeType;

	int32_t	intValue;
	int64_t longValue;
	double doubleValue;
};

struct RegVmCallFrame
{
	RegVmCallFrame() : instruction(0), dataSize(0), regFileSize(0)
	{
	}

	RegVmCallFrame(RegVmCmd *instruction, unsigned dataSize, unsigned regFileSize) : instruction(instruction), dataSize(dataSize), regFileSize(regFileSize)
	{
	}

	RegVmCmd *instruction;
	unsigned dataSize;
	unsigned regFileSize;
};

const char* GetInstructionName(RegVmInstructionCode code);

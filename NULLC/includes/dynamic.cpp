#include "dynamic.h"
#include "../nullc.h"
#include "../nullbind.h"
#include "../nullc_debug.h"

namespace NULLCDynamic
{
	Linker *linker = NULL;

	void RewriteX86(unsigned dest, unsigned src)
	{
		ExternFuncInfo &destFunc = linker->exFunctions[dest];
		ExternFuncInfo &srcFunc = linker->exFunctions[src];

		linker->UpdateFunctionPointer(dest, src);

		if((srcFunc.funcPtrRaw && !destFunc.funcPtrRaw) || (srcFunc.funcPtrWrap && !destFunc.funcPtrWrap))
		{
			nullcThrowError("Internal function cannot be overridden with external function on x86");
			return;
		}
		if((destFunc.funcPtrRaw && !srcFunc.funcPtrRaw) || (destFunc.funcPtrWrap && !srcFunc.funcPtrWrap))
		{
			nullcThrowError("External function cannot be overridden with internal function on x86");
			return;
		}
	}

	void OverrideFunction(NULLCRef dest, NULLCRef src)
	{
		assert(linker);
		if(linker->exTypes[dest.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Destination variable is not a function");
			return;
		}
		if(linker->exTypes[src.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Source variable is not a function");
			return;
		}
		if(dest.typeID != src.typeID)
		{
			nullcThrowError("Cannot convert from '%s' to '%s'", &linker->exSymbols[linker->exTypes[src.typeID].offsetToName], &linker->exSymbols[linker->exTypes[dest.typeID].offsetToName]);
			return;
		}
		ExternFuncInfo &destFunc = linker->exFunctions[((NULLCFuncPtr*)dest.ptr)->id];
		ExternFuncInfo &srcFunc = linker->exFunctions[((NULLCFuncPtr*)src.ptr)->id];
		if(nullcGetCurrentExecutor(NULL) == NULLC_X86)
			RewriteX86(((NULLCFuncPtr*)dest.ptr)->id, ((NULLCFuncPtr*)src.ptr)->id);

		destFunc.vmAddress = srcFunc.vmAddress;
		destFunc.vmCodeSize = srcFunc.vmCodeSize;

		destFunc.regVmAddress = srcFunc.regVmAddress;
		destFunc.regVmCodeSize = srcFunc.regVmCodeSize;
		destFunc.regVmRegisters = srcFunc.regVmRegisters;

		destFunc.funcPtrRaw = srcFunc.funcPtrRaw;
		destFunc.funcPtrWrapTarget = srcFunc.funcPtrWrapTarget;
		destFunc.funcPtrWrap = srcFunc.funcPtrWrap;
	}

	void Override(NULLCRef dest, NULLCArray code)
	{
		assert(linker);
		static unsigned int overrideID = 0;

		if(linker->exTypes[dest.typeID].subCat != ExternTypeInfo::CAT_FUNCTION)
		{
			nullcThrowError("Destination variable is not a function");
			return;
		}

		char tmp[2048];
		char *it = tmp;
		ExternMemberInfo *memberList = &linker->exTypeExtra[linker->exTypes[dest.typeID].memberOffset];
		ExternTypeInfo &returnType = linker->exTypes[memberList[0].type];
		it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "import __last;\r\n%s __override%d(", &linker->exSymbols[0] + returnType.offsetToName, overrideID);

		for(unsigned int i = 0, memberCount = linker->exTypes[dest.typeID].memberCount; i != memberCount; i++)
			it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "%s arg%d%s", &linker->exSymbols[0] + linker->exTypes[memberList[i + 1].type].offsetToName, i, i == memberCount - 1 ? "" : ", ");
		it += NULLC::SafeSprintf(it, 2048 - int(it - tmp), "){ %s }", code.ptr);
		overrideID++;

		if(!nullcCompile(tmp))
		{
			nullcThrowError("%s", nullcGetLastError());
			return;
		}
		char *bytecode = NULL;
		nullcGetBytecodeNoCache(&bytecode);
		if(!nullcLinkCode(bytecode))
		{
			delete[] bytecode;
			nullcThrowError("%s", nullcGetLastError());
			return;
		}
		delete[] bytecode;

		ExternFuncInfo &destFunc = linker->exFunctions[((NULLCFuncPtr*)dest.ptr)->id];
		ExternFuncInfo &srcFunc = linker->exFunctions.back();
		if(nullcGetCurrentExecutor(NULL) == NULLC_X86)
			RewriteX86(((NULLCFuncPtr*)dest.ptr)->id, linker->exFunctions.size() - 1);

		destFunc.vmAddress = srcFunc.vmAddress;
		destFunc.vmCodeSize = srcFunc.vmCodeSize;

		destFunc.regVmAddress = srcFunc.regVmAddress;
		destFunc.regVmCodeSize = srcFunc.regVmCodeSize;
		destFunc.regVmRegisters = srcFunc.regVmRegisters;

		destFunc.funcPtrRaw = srcFunc.funcPtrRaw;
		destFunc.funcPtrWrapTarget = srcFunc.funcPtrWrapTarget;
		destFunc.funcPtrWrap = srcFunc.funcPtrWrap;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.dynamic", NULLCDynamic::funcPtr, name, index)) return false;
bool	nullcInitDynamicModule(Linker* linker)
{
	NULLCDynamic::linker = linker;

	REGISTER_FUNC(OverrideFunction, "override", 0);
	REGISTER_FUNC(Override, "override", 1);

	return true;
}

void	nullcDeinitDynamicModule()
{
	NULLCDynamic::linker = NULL;
}

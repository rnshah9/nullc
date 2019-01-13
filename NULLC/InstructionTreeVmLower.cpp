#include "InstructionTreeVmLower.h"

#include "ExpressionTree.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmCommon.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

typedef InstructionVMLowerContext Context;

void AddCommand(Context &ctx, SynBase *source, VMCmd cmd)
{
	ctx.locations.push_back(source);
	ctx.cmds.push_back(cmd);
}

void Lower(Context &ctx, VmValue *value)
{
	if(VmFunction *function = getType<VmFunction>(value))
	{
		if(function->function && function->function->importModule != NULL)
			return;

		if(function->function && function->function->isPrototype && !function->function->implementation)
			return;

		assert(ctx.currentFunction == NULL);

		ctx.currentFunction = function;

		function->address = ctx.cmds.size();

		if(FunctionData *data = function->function)
		{
			assert(data->argumentsSize < 65536);

			// Stack frame should remain aligned, so its size should multiple of 16
			unsigned size = (data->stackSize + 0xf) & ~0xf;

			// Save previous stack frame, and expand current by shift bytes
			AddCommand(ctx, data->source, VMCmd(cmdPushVTop, (unsigned short)data->argumentsSize, size));
		}

		assert(function->firstBlock);

		for(VmBlock *curr = function->firstBlock; curr; curr = curr->nextSibling)
			Lower(ctx, curr);

		for(unsigned i = 0; i < ctx.fixupPoints.size(); i++)
		{
			Context::FixupPoint &point = ctx.fixupPoints[i];

			assert(point.target);
			assert(point.target->address != ~0u);

			ctx.cmds[point.cmdIndex].argument = point.target->address;
		}

		ctx.fixupPoints.clear();

		function->codeSize = ctx.cmds.size() - function->address;

		ctx.currentFunction = NULL;
	}
	else if(VmBlock *block = getType<VmBlock>(value))
	{
		assert(ctx.currentBlock == NULL);

		ctx.currentBlock = block;

		block->address = ctx.cmds.size();

		for(VmInstruction *curr = block->firstInstruction; curr; curr = curr->nextSibling)
		{
			if(!curr->users.empty())
				continue;

			Lower(ctx, curr);

			if(curr->type.size)
				AddCommand(ctx, block->source, VMCmd(cmdPop, curr->type.size));
		}

		ctx.currentBlock = NULL;
	}
	else if(VmInstruction *inst = getType<VmInstruction>(value))
	{
		switch(inst->cmd)
		{
		case VM_INST_LOAD_BYTE:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdPushChar, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushCharStk, 1, offset->iValue));
			}
			break;
		case VM_INST_LOAD_SHORT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdPushShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushShortStk, 2, offset->iValue));
			}
			break;
		case VM_INST_LOAD_INT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdPushInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushIntStk, 4, offset->iValue));
			}
			break;
		case VM_INST_LOAD_FLOAT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdPushFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushFloatStk, 4, offset->iValue));
			}
			break;
		case VM_INST_LOAD_DOUBLE:
		case VM_INST_LOAD_LONG:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdPushDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdPushDorLStk, 8, offset->iValue));
			}
			break;
		case VM_INST_LOAD_STRUCT:
			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(inst->type.size == 8 ? cmdPushDorL : cmdPushCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->type.size, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(inst->type.size == 8 ? cmdPushDorLStk : cmdPushCmplxStk, (unsigned short)inst->type.size, offset->iValue));
			}
			break;
		case VM_INST_LOAD_IMMEDIATE:
			Lower(ctx, inst->arguments[0]);
			break;
		case VM_INST_STORE_BYTE:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdMovChar, IsLocalScope(constant->container->scope), 1, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovCharStk, 1, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_STORE_SHORT:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdMovShort, IsLocalScope(constant->container->scope), 2, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovShortStk, 2, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_STORE_INT:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdMovInt, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovIntStk, 4, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_STORE_FLOAT:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdMovFloat, IsLocalScope(constant->container->scope), 4, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovFloatStk, 4, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_STORE_LONG:
		case VM_INST_STORE_DOUBLE:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(cmdMovDorL, IsLocalScope(constant->container->scope), 8, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(cmdMovDorLStk, 8, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_STORE_STRUCT:
			Lower(ctx, inst->arguments[2]);

			if(VmConstant *constant = getType<VmConstant>(inst->arguments[0]))
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				assert(offset->iValue == 0);

				unsigned moduleId = constant->container->importModule ? constant->container->importModule->importIndex << 24 : 0;

				AddCommand(ctx, inst->source, VMCmd(inst->arguments[2]->type.size == 8 ? cmdMovDorL : cmdMovCmplx, IsLocalScope(constant->container->scope), (unsigned short)inst->arguments[2]->type.size, constant->iValue + constant->container->offset + moduleId));
			}
			else
			{
				VmConstant *offset = getType<VmConstant>(inst->arguments[1]);

				Lower(ctx, inst->arguments[0]);
				AddCommand(ctx, inst->source, VMCmd(inst->arguments[2]->type.size == 8 ? cmdMovDorLStk : cmdMovCmplxStk, (unsigned short)inst->arguments[2]->type.size, offset->iValue));
			}

			AddCommand(ctx, inst->source, VMCmd(cmdPop, inst->arguments[2]->type.size));
			break;
		case VM_INST_DOUBLE_TO_INT:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdDtoI));
			break;
		case VM_INST_DOUBLE_TO_LONG:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdDtoL));
			break;
		case VM_INST_DOUBLE_TO_FLOAT:
			if(VmConstant *value = getType<VmConstant>(inst->arguments[0]))
			{
				float result = float(value->dValue);

				unsigned target = 0;
				memcpy(&target, &result, sizeof(float));

				AddCommand(ctx, inst->source, VMCmd(cmdPushImmt, target));
			}
			else
			{
				Lower(ctx, inst->arguments[0]);

				AddCommand(ctx, inst->source, VMCmd(cmdDtoF));
			}
			break;
		case VM_INST_INT_TO_DOUBLE:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdItoD));
			break;
		case VM_INST_LONG_TO_DOUBLE:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdLtoD));
			break;
		case VM_INST_INT_TO_LONG:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdItoL));
			break;
		case VM_INST_LONG_TO_INT:
			Lower(ctx, inst->arguments[0]);

			AddCommand(ctx, inst->source, VMCmd(cmdLtoI));
			break;
		case VM_INST_INDEX:
			{
				VmConstant *arrSize = getType<VmConstant>(inst->arguments[0]);
				VmConstant *elementSize = getType<VmConstant>(inst->arguments[1]);
				VmValue *pointer = inst->arguments[2];
				VmValue *index = inst->arguments[3];

				assert(arrSize && elementSize);

				Lower(ctx, pointer);
				Lower(ctx, index);

				AddCommand(ctx, inst->source, VMCmd(cmdIndex, (unsigned short)elementSize->iValue, arrSize->iValue));
			}
			break;
		case VM_INST_INDEX_UNSIZED:
			{
				VmConstant *elementSize = getType<VmConstant>(inst->arguments[0]);
				VmValue *arr = inst->arguments[1];
				VmValue *index = inst->arguments[2];

				assert(elementSize);

				Lower(ctx, arr);
				Lower(ctx, index);

				AddCommand(ctx, inst->source, VMCmd(cmdIndexStk, (unsigned short)elementSize->iValue, 0));
			}
			break;
		case VM_INST_FUNCTION_ADDRESS:
			{
				VmConstant *funcIndex = getType<VmConstant>(inst->arguments[0]);

				assert(funcIndex);

				AddCommand(ctx, inst->source, VMCmd(cmdFuncAddr, funcIndex->iValue));
			}
			break;
		case VM_INST_TYPE_ID:
			{
				VmConstant *typeIndex = getType<VmConstant>(inst->arguments[0]);

				assert(typeIndex);

				AddCommand(ctx, inst->source, VMCmd(cmdPushTypeID, typeIndex->iValue));
			}
			break;
		case VM_INST_SET_RANGE:
			assert(!"not implemented");
			break;
		case VM_INST_JUMP:
			// Check if jump is fall-through
			if(!(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[0]))
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[0])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_Z:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpNZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_JUMP_NZ:
			assert(inst->arguments[0]->type.size == 4);

			Lower(ctx, inst->arguments[0]);

			// Check if one side of the jump is fall-through
			if(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1])
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpZ, ~0u));
			}
			else
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpNZ, ~0u));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[2])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_CALL:
			{
				VmInstruction *target = getType<VmInstruction>(inst->arguments[0]);

				assert(target);

				unsigned short helper = (unsigned short)inst->type.size;

				// Special cases for simple types
				if(inst->type == VmType::Int)
					helper = bitRetSimple | OTYPE_INT;
				else if(inst->type == VmType::Double)
					helper = bitRetSimple | OTYPE_DOUBLE;
				else if(inst->type == VmType::Long)
					helper = bitRetSimple | OTYPE_LONG;

				if(target->cmd == VM_INST_CONSTRUCT)
				{
					VmValue *context = target->arguments[0];
					VmFunction *function = getType<VmFunction>(target->arguments[1]);

					Lower(ctx, context);

					for(int i = int(inst->arguments.size() - 1); i >= 1; i--)
						Lower(ctx, inst->arguments[i]);

					AddCommand(ctx, inst->source, VMCmd(cmdCall, helper, function->function->functionIndex));
				}
				else
				{
					unsigned paramSize = NULLC_PTR_SIZE;

					Lower(ctx, target);

					for(int i = int(inst->arguments.size() - 1); i >= 1; i--)
					{
						Lower(ctx, inst->arguments[i]);

						unsigned size = inst->arguments[i]->type.size;

						paramSize += size > 4 ? size : 4;
					}

					AddCommand(ctx, inst->source, VMCmd(cmdCallPtr, helper, paramSize));
				}
			}
			break;
		case VM_INST_RETURN:
			{
				bool localReturn = ctx.currentFunction->function != NULL;

				if(!inst->arguments.empty())
				{
					VmValue *result = inst->arguments[0];

					if(result->type.size != 0)
						Lower(ctx, result);

					unsigned char operType = OTYPE_COMPLEX;

					if(result->type == VmType::Int)
						operType = OTYPE_INT;
					else if(result->type == VmType::Double)
						operType = OTYPE_DOUBLE;
					else if(result->type == VmType::Long)
						operType = OTYPE_LONG;

					if(result->type.structType && (isType<TypeRef>(result->type.structType) || isType<TypeUnsizedArray>(result->type.structType)))
						AddCommand(ctx, inst->source, VMCmd(cmdCheckedRet, result->type.structType->typeIndex));

					AddCommand(ctx, inst->source, VMCmd(cmdReturn, operType, (unsigned short)localReturn, result->type.size));
				}
				else
				{
					AddCommand(ctx, inst->source, VMCmd(cmdReturn, 0, (unsigned short)localReturn, 0));
				}
			}
			break;
		case VM_INST_YIELD:
			{
				if(!inst->arguments.empty())
				{
					VmValue *result = inst->arguments[0];

					if(result->type.size != 0)
						Lower(ctx, result);

					unsigned char operType = OTYPE_COMPLEX;

					if(result->type == VmType::Int)
						operType = OTYPE_INT;
					else if(result->type == VmType::Double)
						operType = OTYPE_DOUBLE;
					else if(result->type == VmType::Long)
						operType = OTYPE_LONG;

					if(result->type.structType && (isType<TypeRef>(result->type.structType) || isType<TypeUnsizedArray>(result->type.structType)))
						AddCommand(ctx, inst->source, VMCmd(cmdCheckedRet, result->type.structType->typeIndex));

					AddCommand(ctx, inst->source, VMCmd(cmdReturn, operType, 1, result->type.size));
				}
				else
				{
					AddCommand(ctx, inst->source, VMCmd(cmdReturn, 0, 1, 0));
				}
			}
			break;
		case VM_INST_ADD:
			{
				bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

				if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
				{
					Lower(ctx, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdIncI));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdIncD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdIncL));
					else
						assert(!"unknown type");
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdAdd));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdAddD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdAddL));
					else if(inst->type.type == VM_TYPE_POINTER)
						AddCommand(ctx, inst->source, VMCmd(cmdAdd));
					else
						assert(!"unknown type");
				}
			}
			break;
		case VM_INST_SUB:
			{
				bool isContantOneLhs = DoesConstantMatchEither(inst->arguments[0], 1, 1.0f, 1ll);

				if(isContantOneLhs || DoesConstantMatchEither(inst->arguments[1], 1, 1.0f, 1ll))
				{
					Lower(ctx, isContantOneLhs ? inst->arguments[1] : inst->arguments[0]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdDecI));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdDecD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdDecL));
					else
						assert(!"unknown type");
				}
				else
				{
					Lower(ctx, inst->arguments[0]);
					Lower(ctx, inst->arguments[1]);

					if(inst->type == VmType::Int)
						AddCommand(ctx, inst->source, VMCmd(cmdSub));
					else if(inst->type == VmType::Double)
						AddCommand(ctx, inst->source, VMCmd(cmdSubD));
					else if(inst->type == VmType::Long)
						AddCommand(ctx, inst->source, VMCmd(cmdSubL));
					else
						assert(!"unknown type");
				}
			}
			break;
		case VM_INST_MUL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdMul));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdMulD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdMulL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_DIV:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdDiv));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdDivD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdDivL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_POW:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdPow));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdPowD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdPowL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_MOD:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdMod));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdModD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdModL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLess));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdLessD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLessL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdGreater));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdGreaterD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdGreaterL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_LESS_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqual));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqualD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLEqualL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_GREATER_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqual));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqualD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdGEqualL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdEqual));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdEqualD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdEqualL));
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				AddCommand(ctx, inst->source, VMCmd(sizeof(void*) == 4 ? cmdEqual : cmdEqualL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_NOT_EQUAL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqual));
			else if(inst->arguments[0]->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqualD));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdNEqualL));
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				AddCommand(ctx, inst->source, VMCmd(sizeof(void*) == 4 ? cmdNEqual : cmdNEqualL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHL:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdShl));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdShlL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_SHR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdShr));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdShrL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_AND:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitAnd));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitAndL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_OR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitOr));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitOrL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitXor));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitXorL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_LOG_XOR:
			Lower(ctx, inst->arguments[0]);
			Lower(ctx, inst->arguments[1]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLogXor));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLogXorL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_NEG:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdNeg));
			else if(inst->type == VmType::Double)
				AddCommand(ctx, inst->source, VMCmd(cmdNegD));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdNegL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_BIT_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdBitNot));
			else if(inst->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdBitNotL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_LOG_NOT:
			Lower(ctx, inst->arguments[0]);

			if(inst->arguments[0]->type == VmType::Int)
				AddCommand(ctx, inst->source, VMCmd(cmdLogNot));
			else if(inst->arguments[0]->type == VmType::Long)
				AddCommand(ctx, inst->source, VMCmd(cmdLogNotL));
			else if(inst->arguments[0]->type.type == VM_TYPE_POINTER)
				AddCommand(ctx, inst->source, VMCmd(sizeof(void*) == 4 ? cmdLogNot : cmdLogNotL));
			else
				assert(!"unknown type");
			break;
		case VM_INST_CREATE_CLOSURE:
			assert(!"not implemented");
			break;
		case VM_INST_CLOSE_UPVALUES:
			assert(!"not implemented");
			break;
		case VM_INST_CONVERT_POINTER:
			{
				VmValue *pointer = inst->arguments[0];
				VmConstant *typeIndex = getType<VmConstant>(inst->arguments[1]);

				assert(typeIndex);

				Lower(ctx, pointer);

				AddCommand(ctx, inst->source, VMCmd(cmdConvertPtr, typeIndex->iValue));
			}
			break;
		case VM_INST_CHECKED_RETURN:
			assert(!"not implemented");
			break;
		case VM_INST_CONSTRUCT:
		case VM_INST_ARRAY:
			for(int i = int(inst->arguments.size() - 1); i >= 0; i--)
			{
				VmValue *argument = inst->arguments[i];

				assert(argument->type.size % 4 == 0);
				
				if(VmFunction *function = getType<VmFunction>(argument))
					AddCommand(ctx, inst->source, VMCmd(cmdFuncAddr, function->function->functionIndex));
				else
					Lower(ctx, inst->arguments[i]);
			}
			break;
		case VM_INST_EXTRACT:
			AddCommand(ctx, inst->source, VMCmd(cmdNop));
			break;
		case VM_INST_UNYIELD:
			// Check secondary blocks first
			for(unsigned i = 2; i < inst->arguments.size(); i++)
			{
				Lower(ctx, inst->arguments[0]);

				AddCommand(ctx, inst->source, VMCmd(cmdPushImmt, i - 1));
				AddCommand(ctx, inst->source, VMCmd(cmdEqual));

				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[i])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmpNZ, ~0u));
			}

			// jump to entry block by default
			if(!(ctx.currentBlock->nextSibling && ctx.currentBlock->nextSibling == inst->arguments[1]))
			{
				ctx.fixupPoints.push_back(Context::FixupPoint(ctx.cmds.size(), getType<VmBlock>(inst->arguments[1])));
				AddCommand(ctx, inst->source, VMCmd(cmdJmp, ~0u));
			}
			break;
		case VM_INST_BITCAST:
			Lower(ctx, inst->arguments[0]);
			break;
		default:
			assert(!"unknown instruction");
		}
	}
	else if(VmConstant *constant = getType<VmConstant>(value))
	{
		if(constant->type == VmType::Void)
		{
			return;
		}
		else if(constant->type == VmType::Int)
		{
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, constant->iValue));
		}
		else if(constant->type == VmType::Double)
		{
			unsigned data[2];
			memcpy(data, &constant->dValue, 8);

			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[1]));
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type == VmType::Long)
		{
			unsigned data[2];
			memcpy(data, &constant->lValue, 8);

			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[1]));
			AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, data[0]));
		}
		else if(constant->type.type == VM_TYPE_POINTER)
		{
			if(!constant->container)
			{
				assert(constant->iValue == 0);

				AddCommand(ctx, constant->source, VMCmd(cmdPushPtrImmt, 0));
			}
			else
			{
				AddCommand(ctx, constant->source, VMCmd(cmdGetAddr, IsLocalScope(constant->container->scope), constant->iValue + constant->container->offset));
			}
		}
		else if(constant->type.type == VM_TYPE_STRUCT)
		{
			assert(constant->type.size % 4 == 0);

			for(int i = int(constant->type.size / 4) - 1; i >= 0; i--)
				AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, ((unsigned*)constant->sValue)[i]));

			if(constant->type.size == 0)
				AddCommand(ctx, constant->source, VMCmd(cmdPushImmt, 0));
		}
		else
		{
			assert(!"unknown type");
		}
	}
}

void LowerModule(Context &ctx, VmModule *module)
{
	AddCommand(ctx, NULL, VMCmd(cmdJmp, 0));

	for(VmFunction *value = module->functions.head; value; value = value->next)
	{
		if(!value->function)
		{
			module->globalCodeStart = ctx.cmds.size();

			ctx.cmds[0].argument = module->globalCodeStart;
		}

		Lower(ctx, value);
	}
}

VariableData* FindGlobalAt(Context &ctx, unsigned offset)
{
	unsigned targetModuleIndex = offset >> 24;

	if(targetModuleIndex)
		offset = offset & 0xffffff;

	for(unsigned i = 0; i < ctx.ctx.variables.size(); i++)
	{
		VariableData *variable = ctx.ctx.variables[i];

		unsigned variableModuleIndex = variable->importModule ? variable->importModule->importIndex : 0;

		if(IsGlobalScope(variable->scope) && variableModuleIndex == targetModuleIndex && offset >= variable->offset && (offset < variable->offset + variable->type->size || variable->type->size == 0))
			return variable;
	}

	return NULL;
}

void PrintInstructions(Context &ctx, const char *code)
{
	assert(ctx.locations.size() == ctx.cmds.size());

	for(unsigned i = 0; i < ctx.cmds.size(); i++)
	{
		SynBase *source = ctx.locations[i];
		VMCmd &cmd = ctx.cmds[i];

		if(ctx.showSource && source)
		{
			const char *start = source->pos.begin;
			const char *end = start + 1;

			// TODO: handle source locations from imported modules
			while(start > code && *(start - 1) != '\r' && *(start - 1) != '\n')
				start--;

			while(*end && *end != '\r' && *end != '\n')
				end++;

			if (ctx.showAnnotatedSource)
			{
				unsigned startOffset = unsigned(source->pos.begin - start);
				unsigned endOffset = unsigned(source->pos.end - start);

				if(start != ctx.lastStart || startOffset != ctx.lastStartOffset || endOffset != ctx.lastEndOffset)
				{
					fprintf(ctx.file, "%.*s\n", unsigned(end - start), start);

					if (source->pos.end < end)
					{
						for (unsigned i = 0; i < startOffset; i++)
						{
							fprintf(ctx.file, " ");

							if (start[i] == '\t')
								fprintf(ctx.file, "   ");
						}

						for (unsigned i = startOffset; i < endOffset; i++)
						{
							fprintf(ctx.file, "~");

							if (start[i] == '\t')
								fprintf(ctx.file, "~~~");
						}

						fprintf(ctx.file, "\n");
					}

					ctx.lastStart = start;
					ctx.lastStartOffset = startOffset;
					ctx.lastEndOffset = endOffset;
				}
			}
			else
			{
				if(start != ctx.lastStart)
				{
					fprintf(ctx.file, "%.*s\n", unsigned(end - start), start);

					ctx.lastStart = start;
				}
			}
		}

		char buf[256];
		cmd.Decode(buf);

		switch(cmd.cmd)
		{
		case cmdCall:
			if(FunctionData *function = ctx.ctx.functions[cmd.argument])
				fprintf(ctx.file, "// %s (%.*s [%.*s]) param size %d\n", buf, FMT_ISTR(function->name), FMT_ISTR(function->type->name), function->argumentsSize);
			break;
		case cmdFuncAddr:
			if(FunctionData *function = ctx.ctx.functions[cmd.argument])
				fprintf(ctx.file, "// %s (%.*s [%.*s])\n", buf, FMT_ISTR(function->name), FMT_ISTR(function->type->name));
			break;
		case cmdPushTypeID:
			fprintf(ctx.file, "// %s (%.*s)\n", buf, FMT_ISTR(ctx.ctx.types[cmd.argument]->name));
			break;
		case cmdPushChar:
		case cmdPushShort:
		case cmdPushInt:
		case cmdPushFloat:
		case cmdPushDorL:
		case cmdPushCmplx:
		case cmdPushPtr:
		case cmdMovChar:
		case cmdMovShort:
		case cmdMovInt:
		case cmdMovFloat:
		case cmdMovDorL:
		case cmdMovCmplx:
		case cmdGetAddr:
			if(VariableData *global = cmd.flag == 0 ? FindGlobalAt(ctx, cmd.argument) : NULL)
			{
				if(global->importModule)
				{
					if(global->offset == cmd.argument)
						fprintf(ctx.file, "// %s (%.*s [%.*s] from '%.*s')\n", buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
					else
						fprintf(ctx.file, "// %s (inside %.*s [%.*s] from '%.*s')\n", buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name), FMT_ISTR(global->importModule->name));
				}
				else
				{
					if(global->offset == cmd.argument)
						fprintf(ctx.file, "// %s (%.*s [%.*s])\n", buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
					else
						fprintf(ctx.file, "// %s (inside %.*s [%.*s])\n", buf, FMT_ISTR(global->name), FMT_ISTR(global->type->name));
				}
			}
			else
			{
				fprintf(ctx.file, "// %s\n", buf);
			}
			break;
		default:
			fprintf(ctx.file, "// %s\n", buf);
		}
	}
}

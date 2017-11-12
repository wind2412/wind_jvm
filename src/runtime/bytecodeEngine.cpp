/*
 * bytecodeEngine.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/bytecodeEngine.hpp"
#include "wind_jvm.hpp"

/*===----------- StackFrame --------------===*/
StackFrame::StackFrame(Oop *_this, shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev, const std::initializer_list<uint64_t> & list) : method(method), return_pc(return_pc), prev(prev) {	// va_args is: Method's argument. 所有的变长参数的类型全是没有类型的 uint64_t。因此，在**执行 code**的时候才会有类型提升～
	// TODO: set this pointer...
	localVariableTable.insert(localVariableTable.end(), list.begin(), list.end());
}

void StackFrame::clear_all() {					// used with `is_valid()`. if invalid, clear all to reuse this frame.
	this->valid_frame = true;
	stack<uint64_t>().swap(op_stack);				// empty. [Effective STL]
	vector<uint64_t>().swap(localVariableTable);
	method = nullptr;
	return_pc = nullptr;
	// prev not change.
}

/*===------------ BytecodeEngine ---------------===*/

Oop * BytecodeEngine::execute(wind_jvm & jvm) {

	StackFrame & cur_frame = jvm.vm_stack.back();
	uint32_t code_length = cur_frame.method->get_code()->code_length;
	stack<uint64_t> & op_stack = cur_frame.op_stack;
	vector<uint64_t> & localVariableTable = cur_frame.localVariableTable;
	uint8_t *code_begin = cur_frame.method->get_code()->code;

	uint8_t *backup_pc = jvm.pc;
	uint8_t * & pc = jvm.pc;
	pc = code_begin;

	while (pc < code_begin + code_length) {
		int occupied = bccode_map[*pc].second;		// the bytecode takes how many bytes.	// TODO: tableswitch, lookupswitch, wide is NEGATIVE!!!
		switch(*pc) {

			case 0xb2:{		// invokeStatic



				break;
			}





			default:
				std::cerr << "doesn't support bytecode " << bccode_map[*pc].first << " now..." << std::endl;
				assert(false);
		}
		pc += occupied;
	}

	jvm.pc = backup_pc;

	return nullptr;
}


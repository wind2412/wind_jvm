/*
 * bytecodeEngine.hpp
 *
 *  Created on: 2017年11月10日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_BYTECODEENGINE_HPP_
#define INCLUDE_RUNTIME_BYTECODEENGINE_HPP_

#include <vector>
#include <stack>
#include <memory>
#include "oop.hpp"

using std::vector;
using std::stack;
using std::shared_ptr;

class Method;

struct BytecodeEngine {
public:
	static uint64_t execute(stack<uint64_t> & op_stack, vector<uint64_t> & localVariableTable, uint8_t *pc) {

	}
};

struct StackFrame {
public:
	bool valid_frame = true;				// is this frame valid/used ?
	stack<uint64_t> op_stack;			// the inner opcode stack.		// ignore Method::max_stack_size...
	vector<uint64_t> localVariableTable;	// this StackFrame's lvt.		// ignore Method::max_local_size...
	shared_ptr<Method> method;			// the method will be executed in this StackFrame.
	uint8_t *return_pc;					// return_pc to return to the caller's code segment
	StackFrame *prev;					// the caller's StackFrame	// the same as `rbp`
public:
	StackFrame(Oop *_this, shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev) : method(method), return_pc(return_pc), prev(prev) {
		// TODO: set localVariableTable...
		// TODO: set this pointer...
		// TODO: set arguments...
	}
	bool is_valid() { return valid_frame; }
	void set_invalid() { valid_frame = false; }
	void clear_all() {					// used with `is_valid()`. if invalid, clear all to reuse this frame.
		this->valid_frame = true;
		stack<uint64_t>().swap(op_stack);				// empty. [Effective STL]
		vector<uint64_t>().swap(localVariableTable);
		method = nullptr;
		return_pc = nullptr;
		// prev not change.
	}
	void reset_method(shared_ptr<Method> new_method) { this->method = new_method; }
	void reset_return_pc(uint8_t *return_pc) { this->return_pc = return_pc; }
	void execute_method(uint8_t * & pc) {	// 此 StackFrame 的 locals 中的 this, args... 应该在此方法执行前就已经放入了。即，应该交给要 “create this StackFrame” 的 callee 执行！
		if (method->is_native()) {
			pc = nullptr;
			// TODO: native.
			std::cerr << "Doesn't support native now." << std::endl;
			assert(false);
		} else {
			auto code = method->get_code();
			// TODO: support Code attributes......
			if (code->code_length == 0) {
				std::cerr << "empty method??" << std::endl;
				assert(false);		// for test. Is empty method valid ??? I dont know...
			}
			pc = code->code;
			while (pc < code->code + code->code_length) {
				uint64_t return_val = BytecodeEngine::execute(this->op_stack, this->localVariableTable, pc);
				if (method->is_void()) {
					return;		// TODO: is here right ???
				} else {

				}
			}
		}
	}
};





#endif /* INCLUDE_RUNTIME_BYTECODEENGINE_HPP_ */

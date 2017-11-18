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
#include <string>
#include <list>
#include "oop.hpp"

using std::list;
using std::wstring;
using std::vector;
using std::stack;
using std::shared_ptr;

class Method;

struct StackFrame {		// Only a Bean class!
public:
	bool valid_frame = true;				// is this frame valid/used ?
	stack<uint64_t> op_stack;			// the inner opcode stack.		// ignore Method::max_stack_size...
	vector<uint64_t> localVariableTable;	// this StackFrame's lvt.		// ignore Method::max_local_size...
	shared_ptr<Method> method;			// the method will be executed in this StackFrame.
	uint8_t *return_pc;					// return_pc to return to the caller's code segment
	StackFrame *prev;					// the caller's StackFrame	// the same as `rbp`
public:
	StackFrame(Oop *_this, shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev, const list<uint64_t> & list);
	bool is_valid() { return valid_frame; }
	void set_invalid() { valid_frame = false; }
	void clear_all();
	void reset_method(shared_ptr<Method> new_method) { this->method = new_method; }
	void reset_return_pc(uint8_t *return_pc) { this->return_pc = return_pc; }
};

class wind_jvm;

struct BytecodeEngine {
public:
	static Oop * execute(wind_jvm & jvm, StackFrame & cur_frame);
public:	// aux
	static vector<Type> parse_arg_list(const wstring & descriptor);
	static void initial_clinit(shared_ptr<InstanceKlass>, wind_jvm & jvm);
	static bool check_instanceof(shared_ptr<Klass> ref_klass, shared_ptr<Klass> klass);
};


#endif /* INCLUDE_RUNTIME_BYTECODEENGINE_HPP_ */

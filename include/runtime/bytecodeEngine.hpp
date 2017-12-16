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

#define T_BOOLEAN	4
#define T_CHAR		5
#define T_FLOAT		6
#define T_DOUBLE		7
#define T_BYTE		8
#define T_SHORT		9
#define T_INT		10
#define T_LONG		11

class Method;

struct StackFrame {		// Only a Bean class!
public:
	bool valid_frame = true;				// is this frame valid/used ?
	stack<Oop *> op_stack;			// the inner opcode stack.		// ignore Method::max_stack_size...
	vector<Oop *> localVariableTable;	// this StackFrame's lvt.		// ignore Method::max_local_size...
	shared_ptr<Method> method;			// the method will be executed in this StackFrame.
	uint8_t *return_pc;					// return_pc to return to the caller's code segment
	StackFrame *prev;					// the caller's StackFrame	// the same as `rbp`
	bool has_exception = false;
public:
	StackFrame(shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev, const list<Oop *> & list, bool is_native = false);
	bool is_valid() { return valid_frame; }
	void set_invalid() { valid_frame = false; }
	void clear_all();
	void reset_method(shared_ptr<Method> new_method) { this->method = new_method; }
	void reset_return_pc(uint8_t *return_pc) { this->return_pc = return_pc; }
	wstring print_arg_msg(Oop *value);
};

class vm_thread;

struct BytecodeEngine {
public:
	static Oop * execute(vm_thread & thread, StackFrame & cur_frame, int thread_no);
public:	// aux
	static vector<wstring> parse_arg_list(const wstring & descriptor);
	static void initial_clinit(shared_ptr<InstanceKlass>, vm_thread & thread);
	static bool check_instanceof(shared_ptr<Klass> ref_klass, shared_ptr<Klass> klass);
	static wstring get_real_value(Oop *oop);
};


#endif /* INCLUDE_RUNTIME_BYTECODEENGINE_HPP_ */

/*
 * wind_jvm.hpp
 *
 *  Created on: 2017年11月11日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_WIND_JVM_HPP_
#define INCLUDE_WIND_JVM_HPP_

#include "runtime/bytecodeEngine.hpp"
#include "classloader.hpp"
#include <string>
#include <list>

using std::wstring;
using std::list;

class wind_jvm {
	friend BytecodeEngine;
private:
	// TODO: pthread
	wstring main_class_name;
	list<StackFrame> vm_stack;	// 改成了 list...... 因为 vector 的扩容会导致内部迭代器失效......把 vector 作为栈的话，扩容是经常性的...... 故而选用伸缩性更好的 list......
	int rsp;			// offset in [current, valid] vm_stack		// TODO: NEED ?????
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
public:
	wind_jvm(const wstring & main_class_name, const vector<wstring> & argv);	// TODO: arguments! String[] !!!
	void execute() {
		while(!vm_stack.empty()) {		// run over when stack is empty...
			StackFrame & cur_frame = vm_stack.back();
			if (cur_frame.method->is_native()) {
				pc = nullptr;
				// TODO: native.
				std::cerr << "Doesn't support native now." << std::endl;
				assert(false);
			} else {
				auto code = cur_frame.method->get_code();
				// TODO: support Code attributes......
				if (code->code_length == 0) {
					std::cerr << "empty method??" << std::endl;
					assert(false);		// for test. Is empty method valid ??? I dont know...
				}
				pc = code->code;
				Oop * return_val = BytecodeEngine::execute(*this, vm_stack.back());
				if (cur_frame.method->is_void()) {		// TODO: in fact, this can be delete. Because It is of no use.
					assert(return_val == nullptr);
					// do nothing
				} else {
					cur_frame.op_stack.push((uint64_t)return_val);
				}
			}
			vm_stack.pop_back();	// another half push_back() is in wind_jvm() constructor.
		}
	}
	Oop * add_frame_and_execute(shared_ptr<Method> new_method, const std::list<uint64_t> & list) {
		this->vm_stack.push_back(StackFrame(nullptr, new_method, nullptr, nullptr, list));
		Oop * result = BytecodeEngine::execute(*this, this->vm_stack.back());
		this->vm_stack.pop_back();
		return result;
	}
};





#endif /* INCLUDE_WIND_JVM_HPP_ */

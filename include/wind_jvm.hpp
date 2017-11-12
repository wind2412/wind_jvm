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
#include <vector>

using std::wstring;
using std::vector;

class wind_jvm {
	friend BytecodeEngine;
private:
	// TODO: pthread
	wstring main_class_name;
	vector<StackFrame> vm_stack;
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
				Oop * return_val = BytecodeEngine::execute(*this);
				if (cur_frame.method->is_void()) {		// TODO: in fact, this can be delete. Because It is of no use.
					assert(return_val == nullptr);
					// do nothing
				} else {
					cur_frame.op_stack.push((uint64_t)return_val);
				}
			}
		}
	}
};





#endif /* INCLUDE_WIND_JVM_HPP_ */

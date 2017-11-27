/*
 * wind_jvm.hpp
 *
 *  Created on: 2017年11月11日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_WIND_JVM_HPP_
#define INCLUDE_WIND_JVM_HPP_

#include <string>
#include <list>
#include <vector>
#include <memory>
#include <pthread.h>
#include "runtime/bytecodeEngine.hpp"

class wind_jvm;

struct temp {		// pthread aux struct...
	wind_jvm *jvm;
	const vector<wstring> *arg;
};

class wind_jvm {
	friend BytecodeEngine;
private:
	temp p;			// pthread aux struct. must be global!!!
	wstring main_class_name;
	list<StackFrame> vm_stack;	// 改成了 list...... 因为 vector 的扩容会导致内部迭代器失效......把 vector 作为栈的话，扩容是经常性的...... 故而选用伸缩性更好的 list......
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
public:
	wind_jvm(const wstring & main_class_name, const vector<wstring> & argv);	// TODO: arguments! String[] !!!
	void start(const vector<wstring> & argv);
	void execute();
	Oop *add_frame_and_execute(shared_ptr<Method> new_method, const std::list<Oop *> & list);
	MirrorOop *get_caller_class_CallerSensitive();
};





#endif /* INCLUDE_WIND_JVM_HPP_ */

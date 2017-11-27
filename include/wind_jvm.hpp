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
#include "utils/lock.hpp"
#include "runtime/bytecodeEngine.hpp"

class vm_thread;
class wind_jvm;

struct temp {		// pthread aux struct...
	vm_thread *thread;
	std::list<Oop *> *arg;
};

class vm_thread {
	friend BytecodeEngine;
private:
	temp p;			// pthread aux struct. must be global!!!
	shared_ptr<Method> method;
	const std::list<Oop *> & arg;
	list<StackFrame> vm_stack;	// 改成了 list...... 因为 vector 的扩容会导致内部迭代器失效......把 vector 作为栈的话，扩容是经常性的...... 故而选用伸缩性更好的 list......
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
	wind_jvm & jvm;
public:
	vm_thread(shared_ptr<Method> method, const std::list<Oop *> & arg, wind_jvm & jvm) 	// usually `main()` or `run()` method.
																: method(method), arg(arg), pc(0), jvm(jvm) {}
public:
	void launch();
	void start(std::list<Oop *> & arg);
	void execute();
	Oop *add_frame_and_execute(shared_ptr<Method> new_method, const std::list<Oop *> & list);
	MirrorOop *get_caller_class_CallerSensitive();
	void init_and_do_main();
};

class wind_jvm {
	friend BytecodeEngine;
	friend vm_thread;
private:
	static Lock & lock() {
		static Lock lock;
		return lock;
	};		// to lock the threads to prevent the thread's add.
	static bool & inited() {
		static bool inited = false;
		return inited;
	}
	static wstring & main_class_name() {
		static wstring main_class_name;
		return main_class_name;
	};
	static vector<wstring> & argv() {
		static vector<wstring> argv;
		return argv;
	};
	static list<vm_thread> & threads() {
		static list<vm_thread> threads;
		return threads;
	};
public:
	wind_jvm(const wstring & main_class_name, const vector<wstring> & argv);		// TODO: singleton.
};





#endif /* INCLUDE_WIND_JVM_HPP_ */

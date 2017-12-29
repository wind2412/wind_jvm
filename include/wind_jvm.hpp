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
#include "utils/synchronize_wcout.hpp"
#include "runtime/gc.hpp"

class vm_thread;
class wind_jvm;

struct temp {		// pthread aux struct...
	vm_thread *thread;
	std::list<Oop *> *arg;
	InstanceOop *cur_thread_obj;
	volatile bool should_be_stop_first = false;			// if the thread is started by `Thread::start0`, it will be `true`. In other words: only the main thread will be false.
	volatile bool the_first_wait_executed = false;		// the thread start by `Thread::start0` must be stoped at first because we don't know whether there's a gc happens. Dalvik vm is the same method. If the bit is true, then It says the new thread
														// at lease sleep one time: that points out the thread has settled its own thread stack. So we can safely use gc.
};

extern Lock thread_num_lock;
extern int all_thread_num;

enum thread_state {
	Running,
	Waiting,
	Death,
};

class ThreadTable;

class vm_thread {
	friend BytecodeEngine;
	friend GC;
	friend ThreadTable;
private:
	temp p;			// pthread aux struct. must be global!!!
	pthread_t tid;
	thread_state state = Running;
	int monitor_num = 0;

	Method *method;
	std::list<Oop *> arg;
	list<StackFrame> vm_stack;
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
	int thread_no;
public:
	vm_thread(Method *method, const std::list<Oop *> & arg) 	// usually `main()` or `run()` method.
																: method(method), arg(arg), pc(0), tid(0) {
		LockGuard lg(thread_num_lock);
#ifdef DEBUG
		sync_wcout{} << "[*]this thread_no is [" << all_thread_num << "]:" << std::endl;		// delete
#endif
		this->thread_no = all_thread_num ++;
	}
public:
	void launch(InstanceOop * = nullptr);
	void start(std::list<Oop *> & arg);
	Oop *execute();
	Oop *add_frame_and_execute(Method *new_method, const std::list<Oop *> & list);
	MirrorOop *get_caller_class_CallerSensitive();
	void init_and_do_main();
	ArrayOop *get_stack_trace();
	int get_stack_size() { return vm_stack.size(); }
	void set_exception_at_last_second_frame();
public:
	void set_state(thread_state s) { state = s; }
	void monitor_inc() { this->monitor_num ++; }
	void monitor_dec() { this->monitor_num --; }
	int get_monitor_num() { return this->monitor_num; }
    bool is_waited_for_child() { return p.the_first_wait_executed; }
    pthread_t get_tid() { return tid; }
};

class wind_jvm {
	friend BytecodeEngine;
	friend vm_thread;
private:
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
public:
	static pthread_t & gc_thread() {
		static pthread_t gc_thread;
		return gc_thread;
	}
	static int & thread_num() {
		static int thread_num = 0;		// exclude gc thread and init thread, include main thread and `start0` threads.
		return thread_num;
	}
	static Lock & num_lock() {
		static Lock num_lock;
		return num_lock;
	}
	static Lock & lock() {
		static Lock lock;
		return lock;
	};		// to lock the threads to prevent the thread's add.
	static list<vm_thread> & threads() {
		static list<vm_thread> threads;
		return threads;
	};
	static void run(const wstring & main_class_name, const vector<wstring> & argv);
	static void end();
};


extern pthread_mutex_t _all_thread_wait_mutex;
extern pthread_cond_t _all_thread_wait_cond;

void wait_cur_thread(vm_thread *thread);
void wait_cur_thread_and_set_bit(volatile bool *, vm_thread *);
void signal_one_thread();
void signal_all_thread();



#endif /* INCLUDE_WIND_JVM_HPP_ */

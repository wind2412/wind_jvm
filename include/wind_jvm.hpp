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
	volatile bool should_be_stop_first = false;		// 使用 start0() 开启一个新的子线程的时候需要指定此变量为 true！！其他情况不用。这是防止 gc 的时候和 start0() 产生冲突的情况。
	volatile bool the_first_wait_executed = false;	// 一个补丁...... 为了处理由于开启线程 start0 之后，子线程一定会 wait。但是 wait 的时机不一定，如果轮到 main，
											// main 由于运行完毕直接退出，就会无限阻塞了。这样，由于子线程被分配时间片的时机未知，即 执行 wait 的时机未知，因此采用 dalvik 的方法，
											// 即：创建子线程的父进程一定要等到子线程真正执行了 wait 之后才退出 start0.
											// 不过，我又不想使用锁。于是就只能采用多一个变量的方式了。
};

extern Lock thread_num_lock;
extern int all_thread_num;

enum thread_state {
	Running,
	Waiting,
	Death,
};

class vm_thread {
	friend BytecodeEngine;
	friend GC;
private:
	temp p;			// pthread aux struct. must be global!!!
	pthread_t tid;
	thread_state state = Running;
	int monitor_num = 0;				// 此线程在多少个管程之内

	shared_ptr<Method> method;
	std::list<Oop *> arg;
//	const std::list<Oop *> & arg;		// 卧槽我是白痴.....又一次用错了引用...start0 里边是局部变量...我竟然直接通过引用把局部变量引了过来......QAQ
	list<StackFrame> vm_stack;	// 改成了 list...... 因为 vector 的扩容会导致内部迭代器失效......把 vector 作为栈的话，扩容是经常性的...... 故而选用伸缩性更好的 list......
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
	int thread_no;
public:
	vm_thread(shared_ptr<Method> method, const std::list<Oop *> & arg) 	// usually `main()` or `run()` method.
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
	Oop *add_frame_and_execute(shared_ptr<Method> new_method, const std::list<Oop *> & list);
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
	static Lock & lock() {
		static Lock lock;
		return lock;
	};		// to lock the threads to prevent the thread's add.
	static list<vm_thread> & threads() {
		static list<vm_thread> threads;
		return threads;
	};
	static void run(const wstring & main_class_name, const vector<wstring> & argv);		// TODO: can only run for one time ????
};


extern pthread_mutex_t _all_thread_wait_mutex;
extern pthread_cond_t _all_thread_wait_cond;

// 此函数用于唤醒所有线程。(GC)
void wait_cur_thread();
void wait_cur_thread_and_set_bit(volatile bool *);
void signal_all_thread();



#endif /* INCLUDE_WIND_JVM_HPP_ */

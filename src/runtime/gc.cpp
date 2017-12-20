/*
 * gc.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "runtime/gc.hpp"
#include "runtime/thread.hpp"
#include "wind_jvm.hpp"

void GC::init_gc()	// 设置标志位以及目标 vm_threads
{
	// set gc bit
	gc() = true;
	// get all gc roots:
	list<vm_thread> & thread_list = wind_jvm::threads();		 // [x] 注意：这时也可以有新的线程被加进来。想了想，没有必要在线程创建那里加安全点......
	for (auto & thread : thread_list) {
		assert(ThreadTable::is_in(thread.tid));
		// detect the vm_stack is alive?
		if (thread.vm_stack.size() == 0) {		// the vm_thread is end already.
			continue;
		}
		// add it to the target_threads.
		bool ret = target_threads().insert(make_pair(&thread, false)).second;
		assert(ret);
	}
}

void GC::receive_signal(vm_thread *thread) 	// vm_thread 发送一个 ready 信号给 GC 类。此时所有的 vm_thread 应该都进入了 safepoint，即 native 函数以外。
{
	LockGuard lg(gc_lock());

	// 这里的 thread 可以是全新的。因为这里有可能突然有线程的创建。
	target_threads()[thread] = true;

	int total_ready_num = 0;
	for (auto iter : target_threads()) {
		if (iter.second == true) {
			total_ready_num ++;
		} else if (iter.second == false && iter.first->vm_stack.size() == 0) {		// 如果这时候再检查，发现线程已经结束了，就标记为 true 了。
			target_threads()[iter.first] = true;
			total_ready_num ++;
		}
	}

	// all are ready, can gc now!
	if (total_ready_num == target_threads().size()) {
		std::wcout << "hurry!!" << std::endl;
		system_gc();
	}
}

// 这个函数应该被执行在一个新的 GC 进程中。
void GC::system_gc()
{
	assert(false);
}

void GC::set_safepoint_here(vm_thread *thread)
{
	if (GC::gc()) {
		GC::receive_signal(thread);		// send and regist this thread to gc!!
		wait_cur_thread();				// stop this thread!!
	}
}

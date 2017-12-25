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
	LockGuard lg(gc_lock());
	// set gc bit
	if (gc() == true) {		// if has been inited, then here must be during the stop-the-world. so return only.
		return;
	}
	gc() = true;
	// get all gc roots:
	// must use wind_jvm::Lock and GC::Lock.
	LockGuard lg_wind_jvm(wind_jvm::lock());
	list<vm_thread> & thread_list = wind_jvm::threads();		 // [√] 注意：这时也可以有新的线程被加进来。想了想，没有必要在线程创建那里加安全点......
	for (auto & thread : thread_list) {			// [√] target_threads 这个快照，不一定是完整的。因为建立完这个快照之后，也有可能多个线程被创建出来。保证这个快照的正确性的方法，就是让后出现的线程上来就直接自动停止，和 dalvik vm 一样，这样才能够解决两边不一致的缺陷。
		// detect the vm_stack is alive?
		// in fact, `thread.vm_stack.size() == 0` can judge: 1. the vm_thread is waiting (create by `start0`) 2. the thread is dead. Because these two conditions are both: vm_thread.stack().size() == 0.
		pthread_mutex_lock(&_all_thread_wait_mutex);
		thread_state state = thread.state;
		pthread_mutex_unlock(&_all_thread_wait_mutex);

		if (state == Waiting || state == Death /*thread.vm_stack.size() == 0*/) {		// the vm_thread is waiting (create by `start0`) / end already.
			sync_wcout{} << "ignore [" << thread.tid << "] because of: [" << (thread.state == Waiting ? "waiting]" : "death]") << std::endl;
			continue;
		}
		// add it to the target_threads.
		sync_wcout{} << "insert: " << thread.tid << " " << (long)&thread << std::endl;
		bool ret = target_threads().insert(make_pair(&thread, false)).second;
		assert(ret);
	}
	ThreadTable::print_table();
	GC::print_table();
	sync_wcout{} << "init_gc() over." << std::endl;
}

/**
 * 要致力解决的一个重要问题是：由于 init_gc 那里初始化的快照，并不是所有线程的快照。init_gc 那里初始化的时候，是把 “当前程序中的所有线程” 全部加进来。
 * 但是如果就在 init_gc 在运行的时候，突然有一个其他线程产生了 10000 个线程，那么 init_gc 会捕捉不到。因为它只复制了当时的线程表，而新的线程表变动是看不见的。
 * 这样就会引起竞争......多线程 gc 果然难度很大......
 * 矛盾就在于，快照是不可能准确的......
 * 所以，采用了 dalvik vm 的 stop-the-world 解法：
 * —— The easiest way to deal with this is to prevent the new thread from
 * running until the parent says it's okay.
 * 也就是，和 dalvik 不同，我的父线程创建者不能停在 native 上。不过它可以创建子线程，然后创建完的瞬间就让子进程 stop。
 * 然后自己退出 native，走到下一个安全点（即执行此方法结束的时候）自己也 hang up。这样就不会有任何问题了！
 * 然后等到 gc 结束的时候，就会通知子进程 run。这样是 ok 的。我只要保证不会连续 gc 就可以了。
 *
 * 而且需要更改 wind_jvm::threads().push_back(vm_thread(run, {_this})); 的 _this 的引用...... 很麻烦啊......
 */
bool GC::receive_signal(vm_thread *thread) 	// vm_thread 发送一个 ready 信号给 GC 类。此时所有的 vm_thread 应该都进入了 safepoint，即 native 函数以外。
{
	LockGuard lg(gc_lock());

	sync_wcout{} << thread->tid << " is ready." << std::endl;		// delete
	// 这里的 thread 可以是全新的。因为这里有可能突然有线程的创建。
	target_threads()[thread] = true;
	GC::print_table();		// delete

	int total_ready_num = 0;
	for (auto iter : target_threads()) {

		pthread_mutex_lock(&_all_thread_wait_mutex);
		thread_state state = iter.first->state;
		pthread_mutex_unlock(&_all_thread_wait_mutex);

		if (iter.second == true) {
			total_ready_num ++;
		} else if (state == Waiting || state == Death/*iter.second == false && iter.first->vm_stack.size() == 0*/) {		// 如果这时候再检查，发现线程已经结束了，就标记为 true 了。
			target_threads()[iter.first] = true;
			total_ready_num ++;
		} else {
			continue;
		}
	}

	sync_wcout{} << target_threads().size() << std::endl;		// delete

	// all are ready, can gc now! create a new thread~~
	if (total_ready_num == target_threads().size()) {
		pthread_t gc_tid;
		pthread_create(&gc_tid, nullptr, GC::system_gc, nullptr);		// TODO: 这里可以直接转换为 C 指针！和 system_gc 是 static 函数以及 这个调用在 GC 类内调用应该有关系？
		pthread_join(gc_tid, nullptr);
		return false;		// 此线程已经在 gc 的过程中被 pthread_join 阻塞了，因此返回之后不用再阻塞了～
	} else {
		return true;
	}
}

// 这个函数应该被执行在一个新的 GC 进程中。执行此 GC 之前，必须要先进行 stop-the-world。也就是，此函数进行之前，所有除了此 GC 进程之外的线程已经全部停止。
void* GC::system_gc(void *)
{
	// GC-Root and Copy Algorithm
	// 1. get the need-gc-threads:
	// (no need to lock. because there's only this thread in the whole world...)








	// final: 收尾工作，必须进行。
	unordered_map<vm_thread *, bool>().swap(target_threads());
	gc() = false;
	signal_all_thread();

	std::wcout << "gc over!!" << std::endl;		// delete

	return nullptr;
}

void GC::set_safepoint_here(vm_thread *thread)		// 不能强行设置 safepoint !! 如果比如是一堆线程，都要执行 sysout.println()，那么 safepoint 必须设置在不在管程内的时候！！因为如果不这样的话，如果进入了管程的时候就被 stop-the-world 强行中断了的话，那么后边的所有线程将会死锁！！！
{
	LockGuard lg(gc_lock());
	if (GC::gc()) {
		bool need_block = GC::receive_signal(thread);		// send and regist this thread to gc!!
		if (need_block) {
			sync_wcout{} << "block" << std::endl;		// delete
			thread->state = Waiting;
			wait_cur_thread();				// stop this thread!!
			thread->state = Running;
		}
	} else {
		signal_all_thread();		// if not GC, and this thread (maybe) create a new thread using `start0`, then the `new thread` must be hung up. so signal it and start it.
	}
}

void GC::print_table()
{
//#ifdef DEBUG
	sync_wcout::set_switch(true);
	LockGuard lg(GC::gc_lock());
	sync_wcout{} << "===------------- GC Thread Table ----------------===" << std::endl;		// TODO: 这里，sync_wcout 会 dead lock ??????
	for (auto iter : target_threads()) {
		sync_wcout{} << "pthread_t: [" << std::dec << iter.first->tid << "], vm_thread :[" << std::dec << (long)iter.first << "], is ready: ["
				<< std::boolalpha << iter.second << std::dec << "] " << (!iter.first->p.should_be_stop_first ? "(main)" : "") << std::endl;
	}
	sync_wcout{} << "===------------------------------------------===" << std::endl;
//	#endif
}

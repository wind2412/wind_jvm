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
	// 1. set gc bit
	if (gc() == true) {		// if has been inited, then here must be during the stop-the-world. so return only.
		return;
	}
	gc() = true;
	// 2. 初始化快照：
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
			sync_wcout{} << "ignore [" << thread.tid << "] because of: [" << (thread.state == Waiting ? "waiting]" : "death]") << std::endl;	// delete
			continue;
		}
		// add it to the target_threads.
		sync_wcout{} << "insert: " << thread.tid << " " << (long)&thread << std::endl;
		bool ret = target_threads().insert(make_pair(&thread, false)).second;
		assert(ret);
	}
	// 3. signal gc thread:
	std::wcout << "signal gc..." << std::endl;			// delete
	pthread_cond_signal(&gc_cond());
	// 4. print some message...
	ThreadTable::print_table();							// delete
	GC::print_table();									// delete
	sync_wcout{} << "init_gc() over." << std::endl;		// delete
}

/**
 * 要致力解决的一个重要问题是：由于 init_gc 那里初始化的快照，并不是所有线程的快照。init_gc 那里初始化的时候，是把 “当前程序中的所有线程” 全部加进来。
 * 但是如果就在 init_gc 在运行的时候，突然有一个其他线程产生了 10000 个线程，那么 init_gc 会捕捉不到。因为它只复制了当时的线程表，而新的线程表变动是看不见的。
 * 这样就会引起竞争......多线程 gc 果然难度很大......
 * 矛盾就在于，快照是不可能准确的......
 * 为了解决快照不准的问题，必须为 GC 单开线程，来让其轮询。如果不轮询，可能会出现【收集到的快照(内部肯定全是 running 线程)，然后突然有多个线程跑完了，最后一个线程由于处在安全点而被阻塞，这样整个系统就被阻塞了。】这样的状况。
 * 额外地，采用了 dalvik vm 的 stop-the-world 解法：
 * —— The easiest way to deal with this is to prevent the new thread from
 * running until the parent says it's okay.
 * 也就是，和 dalvik 不同，我的父线程创建者不能停在 native 上。不过它可以创建子线程，然后创建完的瞬间就让子进程 stop。
 * 然后自己退出 native，走到下一个安全点（即执行此方法结束的时候）自己也 hang up。这样就不会有任何问题了！
 * 然后等到 gc 结束的时候，就会通知子进程 run。这样是 ok 的。我只要保证不会连续 gc 就可以了。
 *
 * ****而且需要更改 wind_jvm::threads().push_back(vm_thread(run, {_this})); 的 _this 的引用...... 很麻烦啊......****
 */
void GC::detect_ready()
{
	// 轮询，直到所有其他线程都走到了安全点(wait 状态)
	while (true) {
		LockGuard lg(gc_lock());

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

		GC::print_table();		// delete

		if (total_ready_num == target_threads().size()) {		// over!
			return;
		}
	}
}

void *GC::gc_thread(void *)			// 此 gc thread 会一直运行下去。最后会被 真·主线程给 pthread_cancel 掉。当然，cancel 的时机一定是 gc 标志位是 false 的时候，即没在 gc 的时候。
{
	// init `cond` and `mutex` first:
	pthread_cond_init(&gc_cond(), nullptr);
	pthread_mutex_init(&gc_cond_mutex(), nullptr);
	pthread_detach(pthread_self());

	// 无限循环:
	while (true) {

		// 一上来就直接休眠。直到需要 GC 的时候，即 init_gc() 被外界调用的时候，此线程会被 init_gc() 唤醒。
		pthread_mutex_lock(&gc_cond_mutex());
		pthread_cond_wait(&gc_cond(), &gc_cond_mutex());
		pthread_mutex_unlock(&gc_cond_mutex());

		// 苏醒之后，会执行轮询(轮询的时候再拿 GC 锁)，直到 stop-the-world 结束：
		detect_ready();

		// 开始 GC：(由于 stop-the-world，只剩下此一个 Running 线程，所以没有必要在内部取得各种锁了。)
		system_gc();
	}

}

// 这个函数应该被执行在一个新的 GC 进程中。执行此 GC 之前，必须要先进行 stop-the-world。也就是，此函数进行之前，所有除了此 GC 进程之外的线程已经全部停止。
void GC::system_gc()
{
	// GC-Root and Copy Algorithm
	// 1. get the need-gc-threads:
	// (no need to lock. because there's only this thread in the whole world...)








	// final: 收尾工作，必须进行。
	unordered_map<vm_thread *, bool>().swap(target_threads());
	gc() = false;				// no need to lock.
	signal_all_thread();

	std::wcout << "gc over!!" << std::endl;		// delete
}

void GC::set_safepoint_here(vm_thread *thread)		// 不能强行设置 safepoint !! 如果比如是一堆线程，都要执行 sysout.println()，那么 safepoint 必须设置在不在管程内的时候！！因为如果不这样的话，如果进入了管程的时候就被 stop-the-world 强行中断了的话，那么后边的所有线程将会死锁！！！
{													// 改成了直接 block。
	bool gc;
	gc_lock().lock();
	{
		gc = GC::gc();
	}
	gc_lock().unlock();

	if (gc) {
		sync_wcout{} << "block" << std::endl;		// delete
		thread->state = Waiting;
		wait_cur_thread();				// stop this thread!!
		thread->state = Running;
	} else {
		signal_all_thread();		// if not GC, and this thread (maybe) create a new thread using `start0`, then the `new thread` must be hung up. so signal it and start it.
	}
}

void GC::signal_all_patch()
{
	while(true) {
		gc_lock().lock();
		if (!GC::gc()) {		// if not in gc, signal all thread is okay.
			signal_all_thread();
			break;
		}
		gc_lock().unlock();
	}
	gc_lock().unlock();
}

void GC::cancel_gc_thread()
{
	while(true) {
		LockGuard lg(GC::gc_lock());
		if (GC::gc()) {
			continue;
		} else {
			pthread_cancel(wind_jvm::gc_thread());
			break;
		}
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

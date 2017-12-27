/*
 * gc.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "runtime/gc.hpp"
#include "runtime/oop.hpp"
#include "runtime/klass.hpp"
#include "runtime/thread.hpp"
#include "wind_jvm.hpp"
#include "classloader.hpp"
#include "native/java_lang_Class.hpp"
#include "native/java_lang_String.hpp"
#include "utils/utils.hpp"


void GC::init_gc()	// 设置标志位以及目标 vm_threads
{
	LockGuard lg(gc_lock());
	// 1. set gc bit
	if (gc() == true) {		// if has been inited, then here must be during the stop-the-world. so return only.
		return;
	}
	gc() = true;
	// 2. [x] snapshot canceled.
	// 3. signal gc thread:
	std::wcout << "signal gc..." << std::endl;			// delete
	pthread_cond_signal(&gc_cond());
	// 4. print some message...
	ThreadTable::print_table();							// delete
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
		int total_size;

		ThreadTable::get_lock().lock();
		{
			total_size = ThreadTable::get_thread_table().size();
			for (auto & iter : ThreadTable::get_thread_table()) {

//				pthread_mutex_lock(&_all_thread_wait_mutex);		// bug report: dead lock bug from: wind_jvm.cpp::wait_cur_thread().
				thread_state state = std::get<2>(iter.second)->state;
//				pthread_mutex_unlock(&_all_thread_wait_mutex);

				if (state == Waiting || state == Death/*iter.second == false && iter.first->vm_stack.size() == 0*/) {		// 如果这时候再检查，发现线程已经结束了，就标记为 true 了。
					total_ready_num ++;
				} else {
					continue;
				}
			}
		}
		ThreadTable::get_lock().unlock();

		ThreadTable::print_table();		// delete

		if (total_ready_num == total_size) {		// over!
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

// 意为：直接复制对象，并且递归改动新对象的各种指针。最后把新的一堆对象放到 new_oop_map 集合中去。
void GC::recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(Oop * const & origin_oop, unordered_map<Oop *, Oop *> & new_oop_map)
{																	// TODO: 上边！加了个 const 就可以适配 MirrorOop * & 到 Oop * & 了...... const 我一直不明白是什么原理啊...... 还是 too young......
																	// 但是我还要修改它... 所以只能用 const_cast......
	if (origin_oop == nullptr)	return;

	auto iter = new_oop_map.find(origin_oop);			// find the origin_oop has been migrated ? or not ?
	if (iter != new_oop_map.end()) {
		const_cast<Oop *&>(origin_oop) = iter->second;				// 强制修改......
		return;			// has been in the set already. substitute and return.
	}

	// 在 StringTable 中查找需要 hash 函数...... 不能够直接通过地址查找。而每个 oop 都判断是否 String 类，在我这里效率又不高。
	// 因此采用不卸载 StringTable，完整复制的方法节省效率。

	Oop *new_oop = Mempool::copy(*origin_oop);			// should copy the origin in next several conditions.
	new_oop_map.insert(make_pair(origin_oop, new_oop));	// should add to the new_oop_map in next several conditions.
	const_cast<Oop *&>(origin_oop) = new_oop;				// 强制修改......

	if (origin_oop->get_ooptype() == OopType::_BasicTypeOop) {

		// only substitute this oop is okay.
		return;

	} else if (origin_oop->get_ooptype() == OopType::_InstanceOop) {

		// next, add its inner member variables!
		for (auto & iter : ((InstanceOop *)new_oop)->fields) {		// use `&` to modify.
			// if need, substitute the pointer in origin... to the new pointer.
//			std::wcout << iter << " to ";		// delete
			recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(iter, new_oop_map);		// recursively substitute and add into.
//			std::wcout << iter << std::endl;		// delete
		}
		return;


	} else if ((origin_oop->get_ooptype() == OopType::_ObjArrayOop) || (origin_oop->get_ooptype() == OopType::_TypeArrayOop)) {

		// add this oop and its inner elements!
		for (auto & iter : ((ArrayOop *)new_oop)->buf) {		// 验证 origin oop 是否真的替换了...?
			// if need, substitute the pointer in origin... to the new pointer.
//			std::wcout << iter << " to ";		// delete
			recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(iter, new_oop_map);		// recursively substitute and add into.
//			std::wcout << iter << std::endl;		// delete
		}
		return;

	} else {
		assert(false);
	}
}

void GC::klass_inner_oop_gc(Klass *klass, unordered_map<Oop *, Oop *> & new_oop_map)
{
	if (klass->get_type() == ClassType::InstanceClass) {

		InstanceKlass *instanceklass = (InstanceKlass *)klass;

		// for static_fields:
		for (auto & iter : instanceklass->static_fields) {
			// if need, substitute the pointer in origin... to the new pointer.
			recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(iter, new_oop_map);		// recursively add into.
		}
		// for java_loader:
		Oop *mirror = instanceklass->java_loader;
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(mirror, new_oop_map);		// recursively add into.
		instanceklass->java_loader = (MirrorOop *)mirror;

		// for rt_pool:
		auto rt_pool = instanceklass->rt_pool;
		for (auto & _pair : rt_pool->pool) {
			if (_pair.first == 0) {		// this position of the pool has not been parsed.
				continue;
			} else {
				switch (_pair.first) {		// only for String...
					case CONSTANT_String:{
						Oop *addr = boost::any_cast<Oop *>(_pair.second);
						recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(addr, new_oop_map);		// recursively add into.
						_pair.second = boost::any(addr);		// re-pack
						break;
					}
				}
			}
		}

	} else if (klass->get_type() == ClassType::TypeArrayClass || klass->get_type() == ClassType::ObjArrayClass) {

		ArrayKlass *arrklass = (ArrayKlass *)klass;

		// for java_loader:
		Oop *mirror = arrklass->java_loader;
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(mirror, new_oop_map);		// recursively add into.
		arrklass->java_loader = (MirrorOop *)mirror;

	} else {
		assert(false);
	}

//	MirrorOop *oop = klass->java_mirror;		// for debug		// TODO: 这里有 bug ？？ 竟然无法修改？？
//
//	// for java_mirror:
//	recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(klass->java_mirror, new_oop_map);		// recursively add into.
//
//	if (true) {		// for debug
//		std::wcout << klass->name << "'s mirror: from [" << oop << "] to [" << klass->java_mirror << "]." << std::endl;
//	}

	Oop *mirror = klass->java_mirror;						// TODO: 这个是对的......
	// for java_mirror:
	recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(mirror, new_oop_map);		// recursively add into.
	klass->java_mirror = (MirrorOop *)mirror;
}

// 这个函数应该被执行在一个新的 GC 进程中。执行此 GC 之前，必须要先进行 stop-the-world。也就是，此函数进行之前，所有除了此 GC 进程之外的线程已经全部停止。
void GC::system_gc()
{
	// GC-Root and Copy Algorithm
	// get all need-gc-threads(in fact all threads):
	// (**NO NEED TO LOCK**, because there's only this thread in the whole world...)

	// GC-Roots include:
	// 1. InstanceKlass::static_fields
	// 2. InstanceKlass::java_mirror (don't include `get_single_basic_type_mirrors()`'s basic mirror)
	// 3. InstanceKlass::java_loader
	// 4. InstanceKlass::rt_pool's String...
	// 5. InstanceOop::fields
	// 6. ArrayOop::buf
	// #. (gc temporarily do not support `StringTable`'s StringOop. they are all remained.)
	// 7. vm_thread::arg
	// 8. vm_thread::StackFrame[0 ~ the last frame]::localVariableTable
	// 9. vm_thread::StackFrame[0 ~ the last frame]::op_stack
	// 10. ThreadTable

	// 0. create a TEMP new-oop-pool:
	unordered_map<Oop *, Oop *> new_oop_map;		// must be `map/set` instead of list, for getting rid of duplication!!!

	Oop *new_oop;		// global local variable

	// 0.5. first migrate all of the basic type mirrors.
	for (auto & iter : java_lang_class::get_single_basic_type_mirrors()) {
		Oop *mirror = iter.second;
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(mirror, new_oop_map);
		iter.second = (MirrorOop *)mirror;
	}
	// 0.7. 为了避免在 GC 中对照 klass->name 字符串查找，就不实现 StringTable 的卸载了。直接整个复制了一份。
	unordered_set<Oop *, java_string_hash, java_string_equal_to> new_string_table;
	for (auto & iter : java_lang_string::get_string_table()) {
		new_oop = iter;
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(new_oop, new_oop_map);
		new_string_table.insert(new_oop);
	}
	new_string_table.swap(java_lang_string::get_string_table());

	// 1. for all GC-Roots [InstanceKlass]:
	for (auto iter : system_classmap) {
		klass_inner_oop_gc(iter.second, new_oop_map);		// gc the klass
	}
	for (auto iter : MyClassLoader::get_loader().classmap) {
		klass_inner_oop_gc(iter.second, new_oop_map);		// gc the klass
	}
	for (auto iter : MyClassLoader::get_loader().anonymous_klassmap) {
		klass_inner_oop_gc(iter, new_oop_map);		// gc the klass
	}

	// 2. for all GC-Roots [vm_threads]:
	for (auto & thread : wind_jvm::threads()) {
//		std::wcout << "thread: " << thread.tid << ", has " << thread.vm_stack.size() << " frames... "<< std::endl;		// delete
		// 2.3. for thread.args
		for (auto & iter : thread.arg) {
//			std::wcout << "arg: " << iter << std::endl;			// delete
			recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(iter, new_oop_map);
		}
		for (auto & frame : thread.vm_stack) {
			// 2.5. for vm_stack::StackFrame::LocalVariableTable
			for (auto & oop : frame.localVariableTable) {
//				std::wcout << "localVariableTable: " << oop;			// delete
				recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(oop, new_oop_map);
//				std::wcout << " to " << oop;			// delete
			}
			// 2.7. for vm_stack::StackFrame::op_stack
			// stack can't use iter. so make it with another vector...
			list<Oop *> temp;
			while(!frame.op_stack.empty()) {
				Oop *oop = frame.op_stack.top();	frame.op_stack.pop();
				temp.push_front(oop);
			}
			for (auto & oop : temp) {
//				std::wcout << "op_stack: " << oop;		// delete
				recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(oop, new_oop_map);
//				std::wcout << " to " << oop;		// delete
			}
			for (auto & oop : temp) {
				frame.op_stack.push(oop);
			}
		}

	}

	// 2.5. for all GC-Roots: ThreadTable
	for (auto & iter : ThreadTable::get_thread_table()) {
//		std::wcout << "thread: from " << iter.second.second;
		Oop *thread = std::get<1>(iter.second);
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(thread, new_oop_map);		// 由于直接传入 iter.second.second 是 InstanceOop &，和 Oop & 对不上，因此值并不会改变。有待研究。
		std::get<1>(iter.second) = (InstanceOop *)thread;
//		std::wcout << " to " << iter.second.second;
	}

	// 3. create a new oop table and exchange with the global Mempool
	list<Oop *> new_oop_handler_pool;
	for (auto & iter : new_oop_map) {
		new_oop_handler_pool.push_back(iter.second);
	}
	// delete all:
	for (auto iter : Mempool::oop_handler_pool()) {
		delete iter;
	}
	// swap.
	new_oop_handler_pool.swap(Mempool::oop_handler_pool());

	// 4. final: 收尾工作，必须进行。
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
		std::wcout << "block!" << std::endl;			// delete
		wait_cur_thread(thread);						// stop this thread!!
	} else {
		signal_one_thread();		// if not GC, and this thread (maybe) create a new thread using `start0`, then the `new thread` must be hung up. so signal it and start it.
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

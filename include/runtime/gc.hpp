/*
 * gc.hpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_GC_HPP_
#define INCLUDE_RUNTIME_GC_HPP_

#include <unordered_map>
#include "utils/lock.hpp"

using std::unordered_map;

class vm_thread;

class GC {
private:
	static Lock & gc_lock() {
		static Lock gc_lock;
		return gc_lock;
	}
public:
	static bool & gc() {
		static bool gc = false;
		return gc;
	}
	static unordered_map<vm_thread *, bool> & target_threads() {
		static unordered_map<vm_thread *, bool> target_threads;	// 目标 vm_thread，以及它们有无给此 GC 类信号
		return target_threads;
	}
public:
	static void init_gc();		// 设置标志位以及目标 vm_threads
	static void receive_signal(vm_thread *);		// vm_thread 发送一个 ready 信号给 GC 类。此时所有的 vm_thread 应该都进入了 safepoint，即 native 函数以外。
	static void system_gc();		// 应该由另一个新的 GC 线程开启。
	static void set_safepoint_here(vm_thread *);
};


#endif /* INCLUDE_RUNTIME_GC_HPP_ */

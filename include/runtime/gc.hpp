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
public:
	static pthread_cond_t & gc_cond() {
		static pthread_cond_t gc_cond;
		return gc_cond;
	}
	static pthread_mutex_t & gc_cond_mutex() {
		static pthread_mutex_t gc_cond_mutex;
		return gc_cond_mutex;
	}
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
	static void init_gc();			// 设置标志位以及目标 vm_threads
	static void detect_ready();
	static void *gc_thread(void *);	// 应该由另一个新的 GC 线程开启。
	static void system_gc();
	static void set_safepoint_here(vm_thread *);
	static void signal_all_patch();
	static void print_table();
};


#endif /* INCLUDE_RUNTIME_GC_HPP_ */

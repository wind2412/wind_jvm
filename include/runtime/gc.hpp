/*
 * gc.hpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_GC_HPP_
#define INCLUDE_RUNTIME_GC_HPP_

#include <unordered_map>
#include <utility>
#include "utils/lock.hpp"

using std::unordered_map;
using std::pair;
using std::make_pair;

class vm_thread;
class Oop;
class Klass;

class GC {
private:
	static pthread_cond_t & gc_cond() {
		static pthread_cond_t gc_cond;
		return gc_cond;
	}
	static pthread_mutex_t & gc_cond_mutex() {
		static pthread_mutex_t gc_cond_mutex;
		return gc_cond_mutex;
	}
	static void recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(Oop * const & origin_oop, unordered_map<Oop *, Oop *> & new_oop_map);
			// 此函数的第二个参数是新的 oop map。即新的临时堆的句柄空间。意为：unordered_map< origin oop address, new oop address >.
			// return: pair< new oop address, should substitute the origin >.  if `should substitute the origin` == false, that says the oop has been
			// migrated to the `new_oop_map` already. So we need not substitute the pointer.
	static void klass_inner_oop_gc(Klass *klass, unordered_map<Oop *, Oop *> & new_oop_map);
public:
	static Lock & gc_lock() {
		static Lock gc_lock;
		return gc_lock;
	}
	static bool & gc() {
		static bool gc = false;
		return gc;
	}
public:
	static void init_gc();			// 设置标志位以及目标 vm_threads
	static void detect_ready();
	static void *gc_thread(void *);	// 应该由另一个新的 GC 线程开启。
	static void system_gc();
	static void set_safepoint_here(vm_thread *);
	static void signal_all_patch();
	static void cancel_gc_thread();
};


#endif /* INCLUDE_RUNTIME_GC_HPP_ */

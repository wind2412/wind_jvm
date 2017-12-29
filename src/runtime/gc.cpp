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
#include <sched.h>


bool GC::init_gc()
{
	LockGuard lg(gc_lock());
	// 1. set gc bit
	if (gc() == true) {		// if has been inited, then here must be the SIGINT trigged by my settings during the stop-the-world: return only and don't trigger gc.
		return false;
	}
	gc() = true;
	// 2. [x] snapshot canceled.
	// 3. signal gc thread:
	std::wcout << "signal gc..." << std::endl;			// delete
	pthread_cond_signal(&gc_cond());
	// 4. print some message...
	ThreadTable::print_table();							// delete
	sync_wcout{} << "init_gc() over." << std::endl;		// delete
	return true;
}

void GC::detect_ready()
{
//	int i = 0;		// delete
	while (true) {
		LockGuard lg(gc_lock());

		int total_ready_num = 0;
		int total_size;

//		i ++;		// delete

		ThreadTable::get_lock().lock();
		{
			total_size = ThreadTable::get_thread_table().size();
			for (auto & iter : ThreadTable::get_thread_table()) {

//				pthread_mutex_lock(&_all_thread_wait_mutex);		// bug report: dead lock bug from: wind_jvm.cpp::wait_cur_thread().
				thread_state state = std::get<2>(iter.second)->state;
//				pthread_mutex_unlock(&_all_thread_wait_mutex);

				if (state == Waiting || state == Death/*iter.second == false && iter.first->vm_stack.size() == 0*/) {
					total_ready_num ++;
				} else {
					break;
				}
			}
		}
		ThreadTable::get_lock().unlock();

		ThreadTable::print_table();		// delete

		if (total_ready_num == total_size) {		// over!
			return;
		}

//		if (i == 1000) {		// delete
//			assert(false);
//		}

		sched_yield();
	}
}

void *GC::gc_thread(void *)
{
	// init `cond` and `mutex` first:
	pthread_cond_init(&gc_cond(), nullptr);
	pthread_mutex_init(&gc_cond_mutex(), nullptr);

	while (true) {

		pthread_mutex_lock(&gc_cond_mutex());
		pthread_cond_wait(&gc_cond(), &gc_cond_mutex());
		pthread_mutex_unlock(&gc_cond_mutex());

		detect_ready();

		system_gc();
	}

}

void GC::recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(Oop * const & origin_oop, unordered_map<Oop *, Oop *> & new_oop_map)
{
	if (origin_oop == nullptr)	return;

	auto iter = new_oop_map.find(origin_oop);			// find the origin_oop has been migrated ? or not ?
	if (iter != new_oop_map.end()) {
		const_cast<Oop *&>(origin_oop) = iter->second;
		return;			// has been in the set already. substitute and return.
	}

	Oop *new_oop = Mempool::copy(*origin_oop);			// should copy the origin in next several conditions.
	new_oop_map.insert(make_pair(origin_oop, new_oop));	// should add to the new_oop_map in next several conditions.
	const_cast<Oop *&>(origin_oop) = new_oop;

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
		for (auto & iter : ((ArrayOop *)new_oop)->buf) {
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

	Oop *mirror = klass->java_mirror;
	// for java_mirror:
	recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(mirror, new_oop_map);		// recursively add into.
	klass->java_mirror = (MirrorOop *)mirror;
}

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
	// 0.7. I don't want to uninstall all StringTable...
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
		recursive_add_oop_and_its_inner_oops_and_modify_pointers_by_the_way(thread, new_oop_map);
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

	// 4. final: must do this.
	gc() = false;				// no need to lock.
	signal_all_thread();

	std::wcout << "gc over!!" << std::endl;		// delete
}

void GC::set_safepoint_here(vm_thread *thread)
{													// change it to block at once.
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
			pthread_join(wind_jvm::gc_thread(), nullptr);
			break;
		}
	}
}

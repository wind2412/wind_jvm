/*
 * thread.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "runtime/thread.hpp"
#include "utils/synchronize_wcout.hpp"
#include "wind_jvm.hpp"

Lock & ThreadTable::get_lock()
{
	static Lock lock;
	return lock;
}

unordered_map<pthread_t, tuple<int, InstanceOop *, vm_thread *>> & ThreadTable::get_thread_table()
{
	static unordered_map<pthread_t, tuple<int, InstanceOop *, vm_thread *>> thread_table;
	return thread_table;
}

void ThreadTable::add_a_thread(pthread_t tid, InstanceOop *_thread, vm_thread *t)
{
	LockGuard lg(get_lock());

	// C++ 17 新增了一个 insert_or_assign() 比较好...QAQ
//	get_thread_table().insert(make_pair(tid, make_pair(get_thread_table().size(), _thread)));		// Override!!!! because tid maybe the same...?
	int number = get_thread_table().size();
	if (get_thread_table().insert(make_pair(tid, make_tuple(get_thread_table().size(), _thread, t))).second == false) {	// 如果原先已经插入了的话，那么就复用原先的 thread_no.
		number = std::get<0>(get_thread_table()[tid]);
	}
	get_thread_table()[tid] = std::make_tuple(number, _thread, t);		// Override!!!! because tid maybe the same...?
}

void ThreadTable::remove_a_thread(pthread_t tid)
{
	LockGuard lg(get_lock());
	get_thread_table().erase(tid);
}

int ThreadTable::get_threadno(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return std::get<0>((*iter).second);
	}
	return -1;
}

bool ThreadTable::is_in(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return true;
	}
	return false;
}

InstanceOop * ThreadTable::get_a_thread(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return std::get<1>((*iter).second);
	}
	return nullptr;
}

bool ThreadTable::detect_thread_death(pthread_t tid)
{
	LockGuard lg(get_lock());
	int ret = pthread_kill(tid, 0);
	if (ret == 0) {
		return false;
	} else if (ret == ESRCH) {
		return true;
	} else {
		std::wcerr << "wrong!" << std::endl;		// maybe the EINVAL...
		assert(false);
	}
}

void ThreadTable::print_table()
{
//#ifdef DEBUG
	LockGuard lg(get_lock());
	std::wcout << "===------------- ThreadTable ----------------===" << std::endl;
	for (auto iter : get_thread_table()) {
		std::wcout << "pthread_t :[" << iter.first << "], is the [" << std::get<0>(iter.second) <<
				"] thread, Thread Oop address: [" << std::dec << (long)std::get<1>(iter.second) << "]"
				", state:[" << std::get<2>(iter.second)->state << "]" << (!std::get<2>(iter.second)->p.should_be_stop_first ? "(main)" : "")<< std::endl;
	}
	std::wcout << "===------------------------------------------===" << std::endl;
//#endif
}

void ThreadTable::kill_all_except_main_thread(pthread_t main_tid)
{
	for (auto iter : get_thread_table()) {
		if (iter.first == main_tid)	continue;
		else {
//				pthread_cancel(iter.first);
			if (pthread_kill(iter.first, SIGINT) == -1) {
				assert(false);
			}
		}
	}
}

void ThreadTable::wake_up_all_threads_force()
{
	signal_all_thread();
}

/*
 * thread.hpp
 *
 *  Created on: 2017年11月21日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_THREAD_HPP_
#define INCLUDE_RUNTIME_THREAD_HPP_

#include <unordered_map>
#include <memory>
#include <iostream>
#include <cassert>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include "utils/lock.hpp"
#include <utility>

using std::unordered_map;
using std::make_pair;
using std::pair;

class InstanceOop;

class ThreadTable {
private:
	static Lock & get_lock() {
		static Lock lock;
		return lock;
	}
public:
	static unordered_map<pthread_t, pair<int, InstanceOop *>> & get_thread_table() {
		static unordered_map<pthread_t, pair<int, InstanceOop *>> thread_table;
		return thread_table;
	}
public:
	static void add_a_thread(pthread_t tid, InstanceOop *_thread) {
		LockGuard lg(get_lock());
		get_thread_table().insert(make_pair(tid, make_pair(get_thread_table().size(), _thread)));		// Override!!!! because tid maybe the same...?
	}
	static void remove_a_thread(pthread_t tid) {
		LockGuard lg(get_lock());
		get_thread_table().erase(tid);
	}
	static int get_threadno(pthread_t tid) {
		LockGuard lg(get_lock());
		auto iter = get_thread_table().find(tid);
		if (iter != get_thread_table().end()) {
			return (*iter).second.first;
		}
		return -1;
	}
	static InstanceOop * get_a_thread(pthread_t tid) {
		LockGuard lg(get_lock());
		auto iter = get_thread_table().find(tid);
		if (iter != get_thread_table().end()) {
			return (*iter).second.second;
		}
		return nullptr;
	}
	static bool detect_thread_death(pthread_t tid) {
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
	static void print_table() {
#ifdef DEBUG
		sync_wcout{} << "===------------- ThreadTable ----------------===" << std::endl;
		for (auto iter : get_thread_table()) {
			sync_wcout{} << "pthread_t :[" << iter.first << "], is the [" << iter.second.first << "] thread, Thread Oop address: [" << iter.second.second << "]" << std::endl;
		}
		sync_wcout{} << "===------------------------------------------===" << std::endl;
#endif
	}
	static void kill_all_except_main_thread(pthread_t main_tid) {
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
};

#endif /* INCLUDE_RUNTIME_THREAD_HPP_ */

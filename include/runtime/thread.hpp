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
	static Lock & get_lock();
public:
	static unordered_map<pthread_t, pair<int, InstanceOop *>> & get_thread_table();
public:
	static void add_a_thread(pthread_t tid, InstanceOop *_thread);
	static void remove_a_thread(pthread_t tid);
	static int get_threadno(pthread_t tid);
	static InstanceOop * get_a_thread(pthread_t tid);
	static bool detect_thread_death(pthread_t tid);
	static void print_table();
	static void kill_all_except_main_thread(pthread_t main_tid);
};

#endif /* INCLUDE_RUNTIME_THREAD_HPP_ */

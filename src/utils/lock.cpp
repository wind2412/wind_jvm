/*
 * lock.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "utils/lock.hpp"

Lock::Lock() {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);	// 设置递归锁	// 然而其实根本没用到...	// 用到了。在 ThreadTable::print_table() 中...... 如果要在 add_a_thread() 中 print_thread()，那么递归锁是有用的。

	pthread_mutex_init(&mutex, &attr);
}

void Lock::lock() {
	pthread_mutex_lock(&mutex);
}

void Lock::unlock() {
	pthread_mutex_unlock(&mutex);
}

Lock::~Lock() {
	pthread_mutexattr_destroy(&attr);
	pthread_mutex_destroy(&mutex);
}

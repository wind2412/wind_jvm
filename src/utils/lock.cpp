/*
 * lock.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "utils/lock.hpp"

Lock::Lock() {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

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

/*
 * lock.hpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_UTILS_LOCK_HPP_
#define INCLUDE_UTILS_LOCK_HPP_

#include <pthread.h>
#include <boost/noncopyable.hpp>

class Lock : public boost::noncopyable {
private:
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
public:
	Lock() {
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);	// 设置递归锁

		pthread_mutex_init(&mutex, nullptr);
	}
	void lock() {
		pthread_mutex_lock(&mutex);
	}
	void unlock() {
		pthread_mutex_unlock(&mutex);
	}
	~Lock() {
		pthread_mutexattr_destroy(&attr);
		pthread_mutex_destroy(&mutex);
	}
};

class LockGuard {		// RAII
private:
	Lock & lock;
public:
	LockGuard(Lock & lock) : lock(lock){ lock.lock(); }
	~LockGuard() { lock.unlock(); }
};


#endif /* INCLUDE_UTILS_LOCK_HPP_ */

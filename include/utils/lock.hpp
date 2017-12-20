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
	Lock();
	void lock();
	void unlock();
	~Lock();
};

class LockGuard {		// RAII
private:
	Lock & lock;
public:
	LockGuard(Lock & lock) : lock(lock){ lock.lock(); }
	~LockGuard() { lock.unlock(); }
};


#endif /* INCLUDE_UTILS_LOCK_HPP_ */

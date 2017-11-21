/*
 * monitor.hpp
 *
 *  Created on: 2017年11月21日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_UTILS_MONITOR_HPP_
#define INCLUDE_UTILS_MONITOR_HPP_

#include <boost/noncopyable.hpp>
#include <pthread.h>

// this class object, will be embedded into [Oop's header].
// TODO: 每个 unix 函数调用都有返回值！我都没有判断（逃
class Monitor : public boost::noncopyable {
private:
	pthread_mutex_t _mutex;
	pthread_mutexattr_t _attr;
	pthread_cond_t _cond;
public:
	Monitor() {
		pthread_mutexattr_init(&_attr);
		pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);		// 根据 Spec 关于 monitorenter 可重入的问题，即[如果此 thread 持有了 monitor，
																		// 那么它可以重入此 monitor，只不过计数器 +1] 的问题，设置了递归锁。

		pthread_mutex_init(&_mutex, &_attr);
		pthread_cond_init(&_cond, nullptr);
	}
	void enter() {
		pthread_mutex_lock(&_mutex);
	}
	void wait() {
		pthread_cond_wait(&_cond, &_mutex);
	}
	void notify() {
		pthread_cond_signal(&_cond);
	}
	void notify_all() {
		pthread_cond_broadcast(&_cond);
	}
	void leave() {
		pthread_mutex_unlock(&_mutex);
	}
	~Monitor() {
		pthread_mutex_destroy(&_mutex);
		pthread_cond_destroy(&_cond);
	}
};



#endif /* INCLUDE_UTILS_MONITOR_HPP_ */

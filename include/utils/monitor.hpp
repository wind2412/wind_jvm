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
#include <sys/time.h>

// this class object, will be embedded into [Oop's header].
// TODO: 每个 unix 函数调用都有返回值......我都没有判断（逃
class Monitor : public boost::noncopyable {
private:
	pthread_mutex_t _mutex;				// TODO: mutex 需要设置两个来供 cond 使用吗...?
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
	void wait(long timesec) {	// 参数 long 是毫秒级别。				// TODO: 头一次使用 linux 的这些函数...可能有 bug ？
		struct timeval val;		// 毫秒级别。timespec 是纳秒级别。
		gettimeofday(&val, NULL);
		val.tv_usec += timesec;
		// convert:
		struct timespec spec;
		spec.tv_sec = val.tv_sec;
		spec.tv_nsec = val.tv_usec * 1000;	// 毫秒变成纳秒。
		pthread_cond_timedwait(&_cond, &_mutex, &spec);	// 此函数只能接受 spec 参数。即纳秒级。	// 而且这货使用的是绝对时间，不是相对时间！
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
	void force_unlock_when_athrow() {
		pthread_mutex_trylock(&_mutex);		// TODO: QAQ 大神们不要嘲讽 QAQ 多线程我连入门级别都没到 QAQ 且求指导好的写法 QAQ
		pthread_mutex_unlock(&_mutex);		// TODO: 因为 Spec 规范了 unlock 的次数不能小于 lock，而且判断不出到底 lock 还是没有...毕竟我的 cond 也是用同一个 mutex 锁... 不知道对不对 ?? 所以想出了这个方法...轻喷......
	}
	~Monitor() {
		pthread_mutex_destroy(&_mutex);
		pthread_cond_destroy(&_cond);
	}
};



#endif /* INCLUDE_UTILS_MONITOR_HPP_ */

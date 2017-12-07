/*
 * synchronize_wcout.hpp
 *
 *  Created on: 2017年12月8日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_UTILS_SYNCHRONIZE_WCOUT_HPP_
#define INCLUDE_UTILS_SYNCHRONIZE_WCOUT_HPP_

#include <iostream>
#include <sstream>
#include <mutex>

/**
 * from Stack Overflow: https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
 * Though there has lots of object to be generated, it is a good way to sove the synchronized problem ~
 */
class sync_wcout : public std::wostringstream {
private:
	static std::mutex mutex;		// static mutex 锁，非常好的实现～		// TODO: 不过这个 static 的实现不安全吧...应该用 static 函数返回才好吧... 然而 mutex 又是 non-copyable......
public:
	sync_wcout () = default;
	~sync_wcout() {
		std::lock_guard<std::mutex> guard(mutex);
		std::wcout << this->str();		// 在对象销毁的时候，自动输出～ 虽然我也想过，但是没有想到这种实现方法。虽然加锁是万恶之源......
	}
};

#endif /* INCLUDE_UTILS_SYNCHRONIZE_WCOUT_HPP_ */

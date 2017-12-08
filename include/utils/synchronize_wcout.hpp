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

// from Online Source. Unix/Linux only. Used for different thead's output.
#define RESET		"\033[00m"
//#define BLACK		"\033[30m"      	/* Black */
#define RED			"\033[31m"      /* Red */
#define GREEN		"\033[32m"      /* Green */
#define YELLOW		"\033[33m"      /* Yellow */
#define BLUE			"\033[34m"      /* Blue */
#define MAGENTA		"\033[35m"      /* Magenta */
#define CYAN			"\033[36m"      /* Cyan */
//#define WHITE		"\033[37m"      /* White */

/**
 * from Stack Overflow: https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
 * Though there has lots of object to be generated, it is a good way to sove the synchronized problem ~
 */
class sync_wcout : public std::wostringstream {
private:
	static std::mutex & mutex() {		// static mutex 锁，非常好的实现～
		static std::mutex mutex;
		return mutex;
	}
public:
	sync_wcout () = default;
	~sync_wcout();
};

#endif /* INCLUDE_UTILS_SYNCHRONIZE_WCOUT_HPP_ */

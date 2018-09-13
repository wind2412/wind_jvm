/*
 * synchronize_wcout.cpp
 *
 *  Created on: 2017年12月8日
 *      Author: zhengxiaolin
 */

#include "utils/synchronize_wcout.hpp"
#include "runtime/thread.hpp"

sync_wcout::~sync_wcout() {
	std::lock_guard<std::mutex> guard(mutex());
	if (!_switch()) {
		return;
	}
	switch (ThreadTable::get_threadno(pthread_self()) % 7) {
		case 0:
			std::wcout << RESET << this->str();
			break;
		case 1:
			std::wcout << RED << this->str();
			break;
		case 2:
			std::wcout << GREEN << this->str();
			break;
		case 3:
			std::wcout << YELLOW << this->str();
			break;
		case 4:
			std::wcout << BLUE << this->str();
			break;
		case 5:
			std::wcout << MAGENTA << this->str();
			break;
		case 6:
			std::wcout << CYAN << this->str();
			break;
		case -1:
			std::wcout << RESET << this->str();		// for the C++ `int main()`. this is the first thread, which is not created by manually `pthread_create`.
			break;
	}
}

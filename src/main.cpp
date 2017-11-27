/*
 * main.cpp
 *
 *  Created on: 2017年11月11日
 *      Author: zhengxiaolin
 */

#include "wind_jvm.hpp"
#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
	std::wcout.imbue(std::locale(""));
	std::vector<std::wstring> v{ L"wind_jvm", L"1", L"2" };
	wind_jvm::run(L"Test", v);		// TODO: 只能跑一次？？？

	pthread_exit(nullptr);
}

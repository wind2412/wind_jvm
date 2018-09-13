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
//#ifdef DEBUG
	sync_wcout::set_switch(true);
//#endif

	if (argc != 2) {
		std::wcerr << "argc is not 2. please re-run." << std::endl;
		exit(-1);
	}

	wstring program = utf8_to_wstring(std::string(argv[1]));

	std::ios::sync_with_stdio(true);		// keep thread safe?
	std::wcout.imbue(std::locale(""));
	std::vector<std::wstring> v{ L"wind_jvm", L"1", L"2" };
	wind_jvm::run(program, v);

	pthread_exit(nullptr);
}

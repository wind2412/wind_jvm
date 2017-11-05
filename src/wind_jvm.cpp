/*
 * main.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "classloader.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
	std::wcout.imbue(std::locale(""));
	wstring target(L"Example");
	MyClassLoader::get_loader().loadClass(target);
}



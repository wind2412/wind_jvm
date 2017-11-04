/*
 * main.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include <classloader.hpp>

int main(int argc, char *argv[])
{
	wstring target(L"Test");
	MyClassLoader mcl;
	mcl.loadClass(target);
}



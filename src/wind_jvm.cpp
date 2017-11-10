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
	wstring target(L"Test");
	// TODO: load 自己的 class 的时候一定要小心！！一定要把 com.haha.A 变成 com/haha/A 才行！！这个应该在 loadClass 的内部实现。
	MyClassLoader::get_loader().loadClass(target);

	// TODO: 方法区，多线程，堆区，垃圾回收！现在的目标只是 BytecodeExecuteEngine，将来要都加上！！
}



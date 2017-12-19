/*
 * java_lang_Shutdown.cpp
 *
 *  Created on: 2017年12月19日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Shutdown.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"halt0:(I)V",				(void *)&JVM_Halt0},
};

void JVM_Halt0(list<Oop *> & _stack){		// static

	int exitcode = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	// TODO: fully gc.

	exit(exitcode);		// 直接退出！！

}



// 返回 fnPtr.
void *java_lang_shutdown_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


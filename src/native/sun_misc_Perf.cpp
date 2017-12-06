/*
 * sun_misc_Perf.cpp
 *
 *  Created on: 2017年12月6日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_Perf.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "utils/os.hpp"
#include "native/java_lang_String.hpp"

static unordered_map<wstring, void*> methods = {
    {L"createLong:(" STR "IIJ)" BB,				(void *)&JVM_CreateLong},
};

// this method, I simplified it to just run the `run()` method.
void JVM_CreateLong(list<Oop*>& _stack)
{
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	int lvmid = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int mode = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	std::wcout << "(DEBUG) " << java_lang_string::stringOop_to_wstring(str) << ", " << lvmid << ", " << mode << std::endl;

	assert(false);
}

// 返回 fnPtr.
void *sun_misc_perf_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



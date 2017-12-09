/*
 * java_lang_Package.cpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Package.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/utils.hpp"

static unordered_map<wstring, void*> methods = {
    {L"getSystemPackage0:(" STR ")" STR,								(void *)&JVM_GetSystemPackage0},
};

void JVM_GetSystemPackage0(list<Oop *> & _stack){		// static				// TODO:...
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	std::wcout << java_lang_string::stringOop_to_wstring(str) << std::endl;
	assert(false);
}

// 返回 fnPtr.
void *java_lang_package_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


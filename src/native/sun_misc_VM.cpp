/*
 * sun_misc_VM.cpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_VM.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"initialize:()V",				(void *)&JVM_Initialize},
};

void JVM_Initialize(list<Oop *> & _stack){		// static
	// initialize jdk message. I set do nothing.
}



void *sun_misc_vm_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}





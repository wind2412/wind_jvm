/*
 * java_util_concurrent_atomic_AtomicLong.cpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#include "native/java_util_concurrent_atomic_AtomicLong.hpp"

#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"VMSupportsCS8:()Z",				(void *)&JVM_VMSupportsCS8},
};

void JVM_VMSupportsCS8(list<Oop *> & _stack){		// static
	// x86 always returns true.
	_stack.push_back(new IntOop(true));
}



// 返回 fnPtr.
void *java_util_concurrent_atomic_atomicLong_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}




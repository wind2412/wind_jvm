/*
 * sun_reflect_Reflection.cpp
 *
 *  Created on: 2017年11月23日
 *      Author: zhengxiaolin
 */

#include "native/sun_reflect_Reflection.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"getCallerClass:()" CLS,				(void *)&JVM_GetCallerClass},
};

void JVM_GetCallerClass(list<Oop *> & _stack){		// static		// @CallerSensitive ! Very important stack-backtracing method!
	// see: JVM_ENTRY(jclass, JVM_GetCallerClass(JNIEnv* env, int depth)) in openjdk8 : jvm.cpp : 668
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();		// 类似于得到 JNIEnv。
	MirrorOop *result = thread->get_caller_class_CallerSensitive();
	_stack.push_back(result);
}

// 返回 fnPtr.
void *sun_reflect_reflection_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


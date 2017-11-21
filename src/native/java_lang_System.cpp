/*
 * java_lang_System.cpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_
#define SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_

#include "native/java_lang_System.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"

using std::vector;

static unordered_map<wstring, void*> methods = {
    {L"currentTimeMillis:()J",					(void *)&JVM_CurrentTimeMillis},
    {L"nanoTime:()J",							(void *)&JVM_NanoTime},
    {L"arraycopy:(" OBJ L"I" OBJ L"II)V",			(void *)&JVM_ArrayCopy},
    {L"identityHashCode:(" OBJ L")I",			(void *)&JVM_IdentityHashCode},
    {L"initProperties:(" PRO L")" PRO,			(void *)&JVM_InitProperties},
    {L"mapLibraryName:(" STR L")" STR,			(void *)&JVM_MapLibraryName},
    {L"setIn0:(" IPS L")V",						(void *)&JVM_SetIn0},
    {L"setOut0:(" PRS L")V",						(void *)&JVM_SetOut0},
    {L"setErr0:(" PRS L")V",						(void *)&JVM_SetErr0},
};

void JVM_CurrentTimeMillis(list<Oop *> & _stack)		// static
{
	assert(false);
}
void JVM_NanoTime(list<Oop *> & _stack){				// static
	assert(false);
}
void JVM_ArrayCopy(list<Oop *> & _stack){				// static
	Oop *obj1 = (Oop *)_stack.front();	_stack.pop_front();
	IntOop *i1 = (IntOop *)_stack.front();	_stack.pop_front();
	Oop *obj2 = (Oop *)_stack.front();	_stack.pop_front();
	IntOop *i2 = (IntOop *)_stack.front();	_stack.pop_front();
	IntOop *i3 = (IntOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IdentityHashCode(list<Oop *> & _stack){		// static
	Oop *obj = (Oop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_InitProperties(list<Oop *> & _stack){		// static
	InstanceOop *prop = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_MapLibraryName(list<Oop *> & _stack){		// static
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetIn0(list<Oop *> & _stack){		// static
	InstanceOop *inputstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetOut0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetErr0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_system_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


#endif /* SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_ */

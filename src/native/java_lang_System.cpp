/*
 * java_lang_System.cpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_
#define SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_

#include "native/jni.hpp"
#include "native/java_lang_System.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

using std::vector;

static vector<JNINativeMethod> methods = {
	{"registerNatives",	 "()V",			         (void *)&Java_java_lang_system_registerNative},
    {"currentTimeMillis", "()J",                  (void *)&JVM_CurrentTimeMillis},
    {"nanoTime",          "()J",                  (void *)&JVM_NanoTime},
    {"arraycopy",         "(" OBJ "I" OBJ "II)V", (void *)&JVM_ArrayCopy},
    {"identityHashCode",  "(" OBJ ")I",           (void *)&JVM_IdentityHashCode},
    {"initProperties",    "(" PRO ")" PRO,        (void *)&JVM_InitProperties},
    {"mapLibraryName",    "(" STR ")" STR,        (void *)&JVM_MapLibraryName},
    {"setIn0",            "(" IPS ")V",           (void *)&JVM_SetIn0},
    {"setOut0",           "(" PRS ")V",           (void *)&JVM_SetOut0},
    {"setErr0",           "(" PRS ")V",           (void *)&JVM_SetErr0},
};

void Java_java_lang_system_registerNative(stack<Oop *> & _stack)
{
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);		// 把返回值也压到 stack 中去！！只有这样，才能够避免参数擦除的问题！！
}

LongOop *JVM_CurrentTimeMillis(stack<Oop *> & _stack)
{
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
LongOop *JVM_NanoTime(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_ArrayCopy(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	Oop *obj1 = (Oop *)_stack.top();	_stack.pop();
	IntOop *i1 = (IntOop *)_stack.top();	_stack.pop();
	Oop *obj2 = (Oop *)_stack.top();	_stack.pop();
	IntOop *i2 = (IntOop *)_stack.top();	_stack.pop();
	IntOop *i3 = (IntOop *)_stack.top();	_stack.pop();
	assert(false);
}
IntOop *JVM_IdentityHashCode(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	Oop *obj = (Oop *)_stack.top();	_stack.pop();
	assert(false);
}
InstanceOop *JVM_InitProperties(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *prop = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
InstanceOop *JVM_MapLibraryName(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *str = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SetIn0(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *inputstream = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SetOut0(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *printstream = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SetErr0(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *printstream = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_system_search_method(const string & str)
{
	auto iter = std::find_if(methods.begin(), methods.end(), [&str](JNINativeMethod & m){ return m.name == str; });
	if (iter != methods.end()) {
		return (*iter).fnPtr;
	}
	return nullptr;
}


#endif /* SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_ */

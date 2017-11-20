/*
 * java_lang_Object.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/jni.hpp"
#include "native/java_lang_Object.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

using std::vector;

static vector<JNINativeMethod> methods = {
	{"registerNatives",	"()V",		  		 (void *)&Java_java_lang_object_registerNative},
    {"hashCode",    "()I",                    (void *)&JVM_IHashCode},
    {"wait",        "(J)V",                   (void *)&JVM_MonitorWait},
    {"notify",      "()V",                    (void *)&JVM_MonitorNotify},
    {"notifyAll",   "()V",                    (void *)&JVM_MonitorNotifyAll},
    {"clone",       "()" OBJ,                 (void *)&JVM_Clone},
    {"getClass",    "()" KLS,                 (void *)&Java_java_lang_object_getClass},		// I add one line here.
};

void Java_java_lang_object_registerNative(stack<Oop *> & _stack)
{
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}

IntOop *JVM_IHashCode(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_MonitorWait(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	LongOop *l1 = (LongOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_MonitorNotify(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_MonitorNotifyAll(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
Oop *JVM_Clone(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
MirrorOop *Java_java_lang_object_getClass(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_object_search_method(const string & str)
{
	auto iter = std::find_if(methods.begin(), methods.end(), [&str](JNINativeMethod & m){ return m.name == str; });
	if (iter != methods.end()) {
		return (*iter).fnPtr;
	}
	return nullptr;
}

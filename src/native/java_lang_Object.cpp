/*
 * java_lang_Object.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Object.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"hashCode:()I",				(void *)&JVM_IHashCode},
    {L"wait:(J)V",					(void *)&JVM_MonitorWait},
    {L"notify:()V",					(void *)&JVM_MonitorNotify},
    {L"notifyAll:()V",				(void *)&JVM_MonitorNotifyAll},
    {L"clone:()" OBJ,				(void *)&JVM_Clone},
    {L"getClass:()" CLS,				(void *)&Java_java_lang_object_getClass},		// I add one line here.
};

void JVM_IHashCode(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	// hash code: I only use address for it.	// in HotSpot `synchronizer.cpp::get_next_hash()`, condition `hashCode = 4`.
	_stack.push_back(new IntOop((intptr_t)_this));
}
void JVM_MonitorWait(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	LongOop *l1 = (LongOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_MonitorNotify(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_MonitorNotifyAll(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_Clone(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void Java_java_lang_object_getClass(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_object_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

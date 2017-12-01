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
	long val = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	// wait!!
	_this->wait(val);
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
	Oop *_this = _stack.front();	_stack.pop_front();

	if (_this->get_klass()->get_type() == ClassType::InstanceClass) {
		// then, it must implemented the Ljava/lang/Cloneable; interface...
		auto instance_klass = std::static_pointer_cast<InstanceKlass>(_this->get_klass());
		if (!instance_klass->check_interfaces(L"java/lang/Cloneable")) {
			std::wcerr << _this->get_klass()->get_name() << " doesn't implemented the java/lang/Cloneable interface ??" << std::endl;
			assert(false);
		}

		// shallow copy
		InstanceOop *clone = new InstanceOop(*((InstanceOop *)_this));
		_stack.push_back(clone);
#ifdef DEBUG
	std::wcout << "(DEBUG) cloned from obj [" << _this << "] (InstanceOop) to new cloned obj [" << clone << "]." << std::endl;
#endif

	} else if (_this->get_klass()->get_type() == ClassType::TypeArrayClass || _this->get_klass()->get_type() == ClassType::ObjArrayClass) {
		// default implemented java/lang/Cloneable IMPLICITLY. should not check.
		auto array_klass = std::static_pointer_cast<ArrayKlass>(_this->get_klass());

		// shallow copy
		ArrayOop *clone = new ArrayOop(*((ArrayOop *)_this));
		_stack.push_back(clone);

#ifdef DEBUG
	std::wcout << "(DEBUG) cloned from obj [" << _this << "] (ArrayOop) to new cloned obj [" << clone << "]." << std::endl;
#endif
	} else {
		assert(false);
	}
}
void Java_java_lang_object_getClass(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(_this != nullptr);
	_stack.push_back(_this->get_klass()->get_mirror());
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

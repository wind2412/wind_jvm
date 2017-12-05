/*
 * java_lang_Throwable.cpp
 *
 *  Created on: 2017年12月5日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Throwable.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "wind_jvm.hpp"

static unordered_map<wstring, void*> methods = {
    {L"fillInStackTrace:(I)" TRB,							(void *)&JVM_FillInStackTrace},
};

void JVM_FillInStackTrace(list<Oop *> & _stack){		// the int argument is dummy. ignore it~
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	vm_thread & thread = *(vm_thread *)_stack.back();	_stack.pop_back();

	_this->set_field_value(THROWABLE L":backtrace:" OBJ, nullptr);
	_this->set_field_value(THROWABLE L":stackTrace:[Ljava/lang/StackTraceElement;", nullptr);

	ArrayOop *obj = thread.get_stack_trace();		// TODO: 我得出的行号只是粗略值，基于 LineNumberTable。待改进。find_pc_desc_internal()

	_this->set_field_value(THROWABLE L":backtrace:" OBJ, obj);

	_stack.push_back(nullptr);		// return value is of no use.
}

// 返回 fnPtr.
void *java_lang_throwable_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

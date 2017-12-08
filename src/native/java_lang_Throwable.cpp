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
    {L"getStackTraceDepth:()I",								(void *)&JVM_GetStackTraceDepth},
    {L"getStackTraceElement:(I)" STE,						(void *)&JVM_GetStackTraceElement},
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

void JVM_GetStackTraceDepth(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	vm_thread & thread = *(vm_thread *)_stack.back();	_stack.pop_back();

	Oop *obj;
	_this->get_field_value(THROWABLE L":backtrace:" OBJ, &obj);
	assert(obj != nullptr);

	ArrayOop *stacktrace_arr = (ArrayOop *)obj;

	_stack.push_back(new IntOop(stacktrace_arr->get_length()));		// 注意，这里应当取得已经填入的 fillInStackTrace 当时的 stack 深度。现在的深度肯定是不对的。都不知道走过多少个方法，到了这个方法。所以直接取 thread.getstacksize() 肯定不对的。
}

void JVM_GetStackTraceElement(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	int layer = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	Oop *obj;
	_this->get_field_value(THROWABLE L":backtrace:" OBJ, &obj);
	assert(obj != nullptr);

	ArrayOop *stacktrace_arr = (ArrayOop *)obj;

	assert(layer >= 0 && layer < stacktrace_arr->get_length());

	_stack.push_back((*stacktrace_arr)[layer]);
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

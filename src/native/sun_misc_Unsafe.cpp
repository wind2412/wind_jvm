/*
 * sun_misc_Unsafe.cpp
 *
 *  Created on: 2017年11月22日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_Unsafe.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"arrayBaseOffset:(" CLS ")I",				(void *)&JVM_ArrayBaseOffset},
    {L"arrayIndexScale:(" CLS ")I",				(void *)&JVM_ArrayIndexScale},
    {L"addressSize:()I",							(void *)&JVM_AddressSize},
};

void JVM_ArrayBaseOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::cout << "[arrayBaseOffset] " << _array->get_buf_offset() << std::endl;		// delete
	_stack.push_back(new IntOop(_array->get_buf_offset()));
}
void JVM_ArrayIndexScale(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::cout << "[arrayScaleOffset] " << sizeof(intptr_t) << std::endl;		// delete
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}
void JVM_AddressSize(list<Oop *> & _stack){
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}


// 返回 fnPtr.
void *sun_misc_unsafe_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

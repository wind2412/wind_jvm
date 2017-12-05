/*
 * sun_misc_Unsafe.hpp
 *
 *  Created on: 2017年11月22日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_MISC_UNSAFE_HPP_
#define INCLUDE_NATIVE_SUN_MISC_UNSAFE_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_ArrayBaseOffset(list<Oop *> & _stack);
void JVM_ArrayIndexScale(list<Oop *> & _stack);
void JVM_AddressSize(list<Oop *> & _stack);
void JVM_ObjectFieldOffset(list<Oop *> & _stack);
void JVM_GetIntVolatile(list<Oop *> & _stack);
void JVM_CompareAndSwapInt(list<Oop *> & _stack);
void JVM_AllocateMemory(list<Oop *> & _stack);
void JVM_PutLong(list<Oop *> & _stack);
void JVM_GetByte(list<Oop *> & _stack);
void JVM_FreeMemory(list<Oop *> & _stack);
void JVM_GetObjectVolatile(list<Oop *> & _stack);
void JVM_CompareAndSwapObject(list<Oop *> & _stack);
void JVM_CompareAndSwapLong(list<Oop *> & _stack);


void *sun_misc_unsafe_search_method(const wstring & str);

#endif /* INCLUDE_NATIVE_SUN_MISC_UNSAFE_HPP_ */

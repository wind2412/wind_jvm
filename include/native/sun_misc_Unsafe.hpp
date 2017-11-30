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

// 注意：此文件只是为了因为流程必须走过这里而设置。其实内存布局和 jdk 都完全不同，我猜这里我应该用不上。只是随便设置一下。
// 希望日后不会有用到 Unsafe 模块的东西......

void JVM_ArrayBaseOffset(list<Oop *> & _stack);
void JVM_ArrayIndexScale(list<Oop *> & _stack);
void JVM_AddressSize(list<Oop *> & _stack);
void JVM_ObjectFieldOffset(list<Oop *> & _stack);
void JVM_GetIntVolatile(list<Oop *> & _stack);

void *sun_misc_unsafe_search_method(const wstring & str);

#endif /* INCLUDE_NATIVE_SUN_MISC_UNSAFE_HPP_ */

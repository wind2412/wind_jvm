/*
 * java_lang_System.hpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_

#include "runtime/oop.hpp"
#include <stack>

using std::stack;
using std::string;

void Java_java_lang_system_registerNative(stack<Oop *> & _stack);

LongOop *JVM_CurrentTimeMillis(stack<Oop *> & _stack);
LongOop *JVM_NanoTime(stack<Oop *> & _stack);
void JVM_ArrayCopy(stack<Oop *> & _stack);
IntOop *JVM_IdentityHashCode(stack<Oop *> & _stack);
InstanceOop *JVM_InitProperties(stack<Oop *> & _stack);
InstanceOop *JVM_MapLibraryName(stack<Oop *> & _stack);
void JVM_SetIn0(stack<Oop *> & _stack);
void JVM_SetOut0(stack<Oop *> & _stack);
void JVM_SetErr0(stack<Oop *> & _stack);

void *java_lang_system_search_method(const string & str);

#endif /* INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_ */

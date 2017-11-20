/*
 * java_lang_Object.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_

#include "runtime/oop.hpp"
#include <stack>

using std::stack;
using std::string;

void Java_java_lang_object_registerNative(stack<Oop *> & _stack);

IntOop *JVM_IHashCode(stack<Oop *> & _stack);
void JVM_MonitorWait(stack<Oop *> & _stack);
void JVM_MonitorNotify(stack<Oop *> & _stack);
void JVM_MonitorNotifyAll(stack<Oop *> & _stack);
Oop *JVM_Clone(stack<Oop *> & _stack);
MirrorOop *Java_java_lang_object_getClass(stack<Oop *> & _stack);

void *java_lang_object_search_method(const string & str);

#endif /* INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_ */

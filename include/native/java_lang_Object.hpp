/*
 * java_lang_Object.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

void JVM_IHashCode(list<Oop *> & _stack);
void JVM_MonitorWait(list<Oop *> & _stack);
void JVM_MonitorNotify(list<Oop *> & _stack);
void JVM_MonitorNotifyAll(list<Oop *> & _stack);
void JVM_Clone(list<Oop *> & _stack);
void Java_java_lang_object_getClass(list<Oop *> & _stack);

void *java_lang_object_search_method(const wstring & signature);

#endif /* INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_ */

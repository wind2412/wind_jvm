/*
 * java_lang_System.hpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

void JVM_CurrentTimeMillis(list<Oop *> & _stack);
void JVM_NanoTime(list<Oop *> & _stack);
void JVM_ArrayCopy(list<Oop *> & _stack);
void JVM_IdentityHashCode(list<Oop *> & _stack);
void JVM_InitProperties(list<Oop *> & _stack);
void JVM_MapLibraryName(list<Oop *> & _stack);
void JVM_SetIn0(list<Oop *> & _stack);
void JVM_SetOut0(list<Oop *> & _stack);
void JVM_SetErr0(list<Oop *> & _stack);

void *java_lang_system_search_method(const wstring & str);

#endif /* INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_ */

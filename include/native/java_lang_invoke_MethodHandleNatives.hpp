/*
 * java_lang_invoke_MethodHandleNatives.hpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLENATIVES_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLENATIVES_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_GetConstant(list<Oop *> & _stack);
void JVM_Resolve(list<Oop *> & _stack);
void JVM_Expand(list<Oop *> & _stack);
void JVM_Init(list<Oop *> & _stack);
void JVM_MH_ObjectFieldOffset(list<Oop *> & _stack);


void *java_lang_invoke_methodHandleNatives_search_method(const wstring & str);



#endif /* INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLENATIVES_HPP_ */

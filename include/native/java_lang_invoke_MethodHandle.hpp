/*
 * java_lang_invoke_MethodHandle.hpp
 *
 *  Created on: 2017年12月13日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLE_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLE_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_Invoke(list<Oop *> & _stack);
void JVM_InvokeBasic(list<Oop *> & _stack);
void JVM_InvokeExact(list<Oop *> & _stack);


void *java_lang_invoke_methodHandle_search_method(const wstring & str);

class vm_thread;

// aux function
void argument_unboxing(shared_ptr<Method> method, list<Oop *> & args);
InstanceOop *return_val_boxing(Oop *basic_type_oop, vm_thread *thread, const wstring & return_type);


#endif /* INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLE_HPP_ */

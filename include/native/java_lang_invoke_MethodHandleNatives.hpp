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
#include <set>
#include "utils/lock.hpp"

using std::list;
using std::set;

extern Lock member_name_table_lock;
extern set<InstanceOop *> member_name_table;

InstanceOop *find_table_if_match_methodType(InstanceOop *methodType);

void JVM_GetConstant(list<Oop *> & _stack);
void JVM_Resolve(list<Oop *> & _stack);
void JVM_Expand(list<Oop *> & _stack);
void JVM_Init(list<Oop *> & _stack);
void JVM_MH_ObjectFieldOffset(list<Oop *> & _stack);
void JVM_GetMembers(list<Oop *> & _stack);

class vm_thread;

// aux function: public! also for `java_lang_invoke_MethodHandle.hpp`.
wstring get_member_name_descriptor(shared_ptr<InstanceKlass> real_klass, const wstring & real_name, InstanceOop *type);
shared_ptr<Method> get_member_name_target_method(shared_ptr<InstanceKlass> real_klass, const wstring & signature, int ref_kind, vm_thread * = nullptr);

void *java_lang_invoke_methodHandleNatives_search_method(const wstring & str);



#endif /* INCLUDE_NATIVE_JAVA_LANG_INVOKE_METHODHANDLENATIVES_HPP_ */

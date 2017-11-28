/*
 * sun_misc_VM.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_MISC_VM_HPP_
#define INCLUDE_NATIVE_SUN_MISC_VM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_Initialize(list<Oop *> & _stack);



void *sun_misc_vm_search_method(const wstring & signature);




#endif /* INCLUDE_NATIVE_SUN_MISC_VM_HPP_ */

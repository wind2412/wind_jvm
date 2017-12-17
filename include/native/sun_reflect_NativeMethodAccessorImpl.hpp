/*
 * sun_reflect_NativeMethodAccessorImpl.hpp
 *
 *  Created on: 2017年12月17日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_REFLECT_NATIVEMETHODACCESSORIMPL_HPP_
#define INCLUDE_NATIVE_SUN_REFLECT_NATIVEMETHODACCESSORIMPL_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_Invoke0(list<Oop *> & _stack);



void *sun_reflect_nativeMethodAccessorImpl_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_SUN_REFLECT_NATIVEMETHODACCESSORIMPL_HPP_ */

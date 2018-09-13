/*
 * sun_reflect_NativeConstructorAccessorImpl.hpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_REFLECT_NATIVECONSTRUCTORACCESSORIMPL_HPP_
#define INCLUDE_NATIVE_SUN_REFLECT_NATIVECONSTRUCTORACCESSORIMPL_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_NewInstanceFromConstructor(list<Oop *> & _stack);



void *sun_reflect_nativeConstructorAccessorImpl_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_SUN_REFLECT_NATIVECONSTRUCTORACCESSORIMPL_HPP_ */

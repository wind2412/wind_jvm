/*
 * sun_reflect_Reflection.hpp
 *
 *  Created on: 2017年11月23日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_REFLECT_REFLECTION_HPP_
#define INCLUDE_NATIVE_SUN_REFLECT_REFLECTION_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_GetCallerClass(list<Oop *> & _stack);

void *sun_reflect_reflection_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_SUN_REFLECT_REFLECTION_HPP_ */

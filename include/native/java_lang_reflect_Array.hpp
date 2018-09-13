/*
 * java_lang_reflect_Array.hpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_REFLECT_ARRAY_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_REFLECT_ARRAY_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_NewArray(list<Oop *> & _stack);


void *java_lang_reflect_array_search_method(const wstring & str);





#endif /* INCLUDE_NATIVE_JAVA_LANG_REFLECT_ARRAY_HPP_ */

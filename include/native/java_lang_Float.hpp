/*
 * java_lang_Float.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_FLOAT_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_FLOAT_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_FloatToRawIntBits(list<Oop *> & _stack);



void *java_lang_float_search_method(const wstring & signature);

#endif /* INCLUDE_NATIVE_JAVA_LANG_FLOAT_HPP_ */

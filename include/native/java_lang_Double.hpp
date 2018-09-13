/*
 * java_lang_Double.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_DOUBLE_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_DOUBLE_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_DoubleToRawLongBits(list<Oop *> & _stack);
void JVM_LongBitsToDouble(list<Oop *> & _stack);



void *java_lang_double_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_JAVA_LANG_DOUBLE_HPP_ */

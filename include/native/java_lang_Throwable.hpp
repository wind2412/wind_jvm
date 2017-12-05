/*
 * java_lang_Throwable.hpp
 *
 *  Created on: 2017年12月5日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_THROWABLE_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_THROWABLE_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_FillInStackTrace(list<Oop *> & _stack);

void *java_lang_throwable_search_method(const wstring & str);



#endif /* INCLUDE_NATIVE_JAVA_LANG_THROWABLE_HPP_ */

/*
 * java_lang_Runtime.hpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_RUNTIME_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_RUNTIME_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_AvailableProcessors(list<Oop*>& _stack);



void *java_lang_runtime_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_JAVA_LANG_RUNTIME_HPP_ */

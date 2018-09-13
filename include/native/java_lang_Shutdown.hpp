/*
 * java_lang_Shutdown.hpp
 *
 *  Created on: 2017年12月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_SHUTDOWN_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_SHUTDOWN_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_Halt0(list<Oop *> & _stack);


void *java_lang_shutdown_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_JAVA_LANG_SHUTDOWN_HPP_ */

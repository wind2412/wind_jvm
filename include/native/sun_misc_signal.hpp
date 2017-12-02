/*
 * sun_misc_signal.hpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_MISC_SIGNAL_HPP_
#define INCLUDE_NATIVE_SUN_MISC_SIGNAL_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_FindSignal(list<Oop *> & _stack);
void JVM_Handle0(list<Oop *> & _stack);



void *sun_misc_signal_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_SUN_MISC_SIGNAL_HPP_ */

/*
 * sun_misc_Perf.hpp
 *
 *  Created on: 2017年12月6日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_MISC_PERF_HPP_
#define INCLUDE_NATIVE_SUN_MISC_PERF_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_CreateLong(list<Oop*>& _stack);



void *sun_misc_perf_search_method(const wstring & signature);





#endif /* INCLUDE_NATIVE_SUN_MISC_PERF_HPP_ */

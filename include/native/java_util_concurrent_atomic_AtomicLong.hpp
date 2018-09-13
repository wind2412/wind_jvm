/*
 * java_util_concurrent_atomic_AtomicLong.hpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_HPP_
#define INCLUDE_NATIVE_JAVA_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_HPP_


#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_VMSupportsCS8(list<Oop *> & _stack);



void *java_util_concurrent_atomic_atomicLong_search_method(const wstring & signature);





#endif /* INCLUDE_NATIVE_JAVA_UTIL_CONCURRENT_ATOMIC_ATOMICLONG_HPP_ */

/*
 * sun_misc_URLClassPath.hpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_SUN_MISC_URLCLASSPATH_HPP_
#define INCLUDE_NATIVE_SUN_MISC_URLCLASSPATH_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_GetLookupCacheURLs(list<Oop *> & _stack);



void *sun_misc_urlClassPath_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_SUN_MISC_URLCLASSPATH_HPP_ */

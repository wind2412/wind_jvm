/*
 * java_lang_Package.hpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_PACKAGE_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_PACKAGE_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_GetSystemPackage0(list<Oop *> & _stack);



void *java_lang_package_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_JAVA_LANG_PACKAGE_HPP_ */

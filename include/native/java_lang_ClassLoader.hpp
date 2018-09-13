/*
 * java_lang_ClassLoader.hpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_CLASSLOADER_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_CLASSLOADER_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_FindLoadedClass(list<Oop *> & _stack);
void JVM_FindBootStrapClass(list<Oop *> & _stack);
void JVM_DefineClass1(list<Oop *> & _stack);



void *java_lang_classLoader_search_method(const wstring & signature);




#endif /* INCLUDE_NATIVE_JAVA_LANG_CLASSLOADER_HPP_ */

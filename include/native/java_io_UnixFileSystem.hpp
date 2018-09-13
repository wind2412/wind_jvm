/*
 * java_io_UnixFileSystem.hpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_IO_UNIXFILESYSTEM_HPP_
#define INCLUDE_NATIVE_JAVA_IO_UNIXFILESYSTEM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_UFS_InitIDs(list<Oop *> & _stack);
void JVM_Canonicalize0(list<Oop *> & _stack);
void JVM_GetBooleanAttributes0(list<Oop *> & _stack);



void *java_io_unixFileSystem_search_method(const wstring & signature);





#endif /* INCLUDE_NATIVE_JAVA_IO_UNIXFILESYSTEM_HPP_ */

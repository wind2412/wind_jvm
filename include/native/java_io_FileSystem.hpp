/*
 * java_io_FileSystem.hpp
 *
 *  Created on: 2017年12月7日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_IO_FILESYSTEM_HPP_
#define INCLUDE_NATIVE_JAVA_IO_FILESYSTEM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_GetLength(list<Oop *> & _stack);



void *java_io_fileSystem_search_method(const wstring & signature);




#endif /* INCLUDE_NATIVE_JAVA_IO_FILESYSTEM_HPP_ */

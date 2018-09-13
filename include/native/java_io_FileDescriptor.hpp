/*
 * java_io_FileDescriptor.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_IO_FILEDESCRIPTOR_HPP_
#define INCLUDE_NATIVE_JAVA_IO_FILEDESCRIPTOR_HPP_


#include "runtime/oop.hpp"
#include <list>

using std::list;

void JVM_FD_InitIDs(list<Oop *> & _stack);



void *java_io_fileDescriptor_search_method(const wstring & signature);



#endif /* INCLUDE_NATIVE_JAVA_IO_FILEDESCRIPTOR_HPP_ */

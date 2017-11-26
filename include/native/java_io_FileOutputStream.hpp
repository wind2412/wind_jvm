/*
 * java_io_FileOutputStream.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_IO_FILEOUTPUTSTREAM_HPP_
#define INCLUDE_NATIVE_JAVA_IO_FILEOUTPUTSTREAM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

void JVM_FOS_InitIDs(list<Oop *> & _stack);



void *java_io_fileOutputStream_search_method(const wstring & signature);




#endif /* INCLUDE_NATIVE_JAVA_IO_FILEOUTPUTSTREAM_HPP_ */

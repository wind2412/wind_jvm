/*
 * java_io_FileInputStream.hpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_IO_FILEINPUTSTREAM_HPP_
#define INCLUDE_NATIVE_JAVA_IO_FILEINPUTSTREAM_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

void JVM_FIS_InitIDs(list<Oop *> & _stack);



void *java_io_fileInputStream_search_method(const wstring & signature);





#endif /* INCLUDE_NATIVE_JAVA_IO_FILEINPUTSTREAM_HPP_ */

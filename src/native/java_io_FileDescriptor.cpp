/*
 * java_io_FiileDescriptor.cpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#include "native/java_io_FileDescriptor.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"initIDs:()V",				(void *)&JVM_FD_InitIDs},
};

void JVM_FD_InitIDs(list<Oop *> & _stack){		// static

	// 此方法旨在设置 FileDescriptor::field 的偏移大小......
	// 我并不知道这有什么意义......
	// 所以我 do nothing...

}



// 返回 fnPtr.
void *java_io_fileDescriptor_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



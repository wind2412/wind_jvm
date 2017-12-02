/*
 * java_io_UnixFileSystem.cpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#include "native/java_io_UnixFileSystem.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"initIDs:()V",				(void *)&JVM_UFS_InitIDs},
};

void JVM_UFS_InitIDs(list<Oop *> & _stack){		// static
	// TODO: 到了这一步我还不明白这方法有什么用......有待研究...
}



// 返回 fnPtr.
void *java_io_unixFileSystem_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


/*
 * java_io_FileOutputStream.cpp
 *
 *  Created on: 2017年11月27日
 *      Author: zhengxiaolin
 */

#include "native/java_io_FileOutputStream.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include <unistd.h>

#define MAX_BUF_SIZE		4096

static unordered_map<wstring, void*> methods = {
    {L"initIDs:()V",				(void *)&JVM_FOS_InitIDs},
    {L"writeBytes:([BIIZ)V",		(void *)&JVM_WriteBytes},
};

void JVM_FOS_InitIDs(list<Oop *> & _stack){		// static

	// 此方法旨在设置 FileDescriptor::field 的偏移大小......
	// 我并不知道这有什么意义......
	// 所以我 do nothing...

}

void JVM_WriteBytes(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	TypeArrayOop *bytes = (TypeArrayOop *)_stack.front(); _stack.pop_front();
	int offset = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int len = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	bool append = (bool)((IntOop *)_stack.front())->value;	_stack.pop_front();		// in linux/unix, append is of no use. because `append` is `open()`'s property in *nix. It's only useful for windows.

	Oop *oop;
	// get the unix fd.
	_this->get_field_value(FILEOUTPUTSTREAM L":fd:Ljava/io/FileDescriptor;", &oop);
	((InstanceOop *)oop)->get_field_value(FILEDESCRIPTOR L":fd:I", &oop);
	int fd = ((IntOop *)oop)->value;

	assert(bytes->get_length() > offset && bytes->get_length() >= (offset + len));		// ArrayIndexOutofBoundException

	char *buf = new char[len];

	for (int i = offset, j = 0; i < offset + len; i ++, j ++) {
		buf[j] = (char)((IntOop *)(*bytes)[i])->value;
	}

	if (write(fd, buf, len) == -1) {			// TODO: 对中断进行处理！！
		assert(false);
	}

	delete[] buf;
}



// 返回 fnPtr.
void *java_io_fileOutputStream_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



/*
 * java_io_FileInputStream.cpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#include "native/java_io_FileInputStream.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "classloader.hpp"

static unordered_map<wstring, void*> methods = {
    {L"initIDs:()V",				(void *)&JVM_FIS_InitIDs},
    {L"open0:(" STR ")V",		(void *)&JVM_Open0},
    {L"readBytes:([BII)I",		(void *)&JVM_ReadBytes},
    {L"close0:()V",				(void *)&JVM_Close0},
};

void JVM_FIS_InitIDs(list<Oop *> & _stack){		// static

	// 此方法旨在设置 FileDescriptor::field 的偏移大小......
	// 我并不知道这有什么意义......
	// 所以我 do nothing...

}

void JVM_Open0(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();

	std::string backup_str = wstring_to_utf8(java_lang_string::stringOop_to_wstring(str));
	const char *filename = backup_str.c_str();

#ifdef DEBUG
	sync_wcout{} << "open filename: [" << filename << "]." << std::endl;
#endif

	// use O_RDONLY to open the file!!! (of FileInputStream, 1. readOnly, 2. only read File, not dir!)

	int fd;
	do {
		fd = open(filename, O_RDONLY, 0666);
	} while (fd == -1 && errno == EINTR);		// prevent from being interrupted from SIGINT.

	if (fd == -1)	assert(false);

	int ret;
	struct stat64 stat;
	do {
		ret = fstat64(fd, &stat);
	} while (ret == -1 && errno == EINTR);

	if (ret == -1)	assert(false);

	if (S_ISDIR(stat.st_mode)) {		// check whether `fd` is a dir. FileInputStream.open() can only open the `file`, not `dir`.
		close(fd);
		// throw FileNotFoundException: xxx is a directory.

		// get the exception klass
		auto excp_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/io/FileNotFoundException"));
		// get current thread
		vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();
		// make a message
		std::string msg0(filename);
		wstring msg = utf8_to_wstring(msg0);
		msg += L" (Is a directory)";
		// go!
		native_throw_Exception(excp_klass, thread, _stack, msg);	// the exception obj has been save into the `_stack`!

		return;
	}

	// inject into FileInputStream.fd !!
	Oop *result;
	_this->get_field_value(FILEINPUTSTREAM L":fd:Ljava/io/FileDescriptor;", &result);
	Oop *real_fd;
	((InstanceOop *)result)->get_field_value(FILEDESCRIPTOR L":fd:I", &real_fd);

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) open a file [" << filename << "], and inject real fd: [" << fd << "] to substitude the old garbage value fd: [" << ((IntOop *)real_fd)->value << "]." << std::endl;
#endif

	((IntOop *)real_fd)->value = fd;		// inject the real fd into it!

}

void JVM_ReadBytes(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	TypeArrayOop *bytes = (TypeArrayOop *)_stack.front(); _stack.pop_front();
	int offset = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int len = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	bool append = (bool)((IntOop *)_stack.front())->value;	_stack.pop_front();		// in linux/unix, append is of no use. because `append` is `open()`'s property in *nix. It's only useful for windows.

	Oop *oop;
	// get the unix fd.
	_this->get_field_value(FILEINPUTSTREAM L":fd:Ljava/io/FileDescriptor;", &oop);
	((InstanceOop *)oop)->get_field_value(FILEDESCRIPTOR L":fd:I", &oop);
	int fd = ((IntOop *)oop)->value;

	assert(bytes->get_length() > offset && bytes->get_length() >= (offset + len));		// ArrayIndexOutofBoundException

	char *buf = new char[len];

	int ret;
	if ((ret = read(fd, buf, len)) == -1) {		// TODO: 对中断进行处理！！
		assert(false);
	}

	if (ret == 0) {		// EOF
		_stack.push_back(new IntOop(-1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) meet EOF of fd: [" << fd << "]!" << std::endl;
#endif
	} else {				// Not EOF
		for (int i = offset, j = 0; i < offset + ret; i ++, j ++) {		// I think it should be `offset + ret`, not `offset + len`.
			((IntOop *)(*bytes)[i])->value = buf[j];
		}
		_stack.push_back(new IntOop(ret));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) read fd: [" << fd << "] for [" << ret << "] bytes." << std::endl;
#endif
	}

	delete[] buf;


}

void JVM_Close0(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();

	Oop *oop;
	// get the unix fd.
	_this->get_field_value(FILEINPUTSTREAM L":fd:Ljava/io/FileDescriptor;", &oop);
	((InstanceOop *)oop)->get_field_value(FILEDESCRIPTOR L":fd:I", &oop);
	int fd = ((IntOop *)oop)->value;

	assert(fd != -1);

	if (close(fd) == -1) {			// TODO: 对中断进行设置!
		assert(false);
	}

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) close a file fd: [" << fd << "]." << std::endl;
#endif

}





// 返回 fnPtr.
void *java_io_fileInputStream_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



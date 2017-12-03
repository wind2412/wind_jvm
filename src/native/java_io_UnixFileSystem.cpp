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
#include "native/java_lang_String.hpp"
#include <boost/filesystem.hpp>
#include "utils/utils.hpp"
#include <sys/stat.h>

static unordered_map<wstring, void*> methods = {
    {L"initIDs:()V",								(void *)&JVM_UFS_InitIDs},
    {L"canonicalize0:(" STR ")" STR,				(void *)&JVM_Canonicalize0},
    {L"getBooleanAttributes0:(" FLE ")I",			(void *)&JVM_GetBooleanAttributes0},
};

void JVM_UFS_InitIDs(list<Oop *> & _stack){		// static
	// TODO: 到了这一步我还不明白这方法有什么用......有待研究...
}

void JVM_Canonicalize0(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();

	_stack.push_back(java_lang_string::intern(boost::filesystem::canonical(java_lang_string::stringOop_to_wstring(str)).wstring()));

#ifdef DEBUG
	std::wcout << "(DEBUG) canonical path of [" << java_lang_string::stringOop_to_wstring(str) << "] is [" << java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.back()) << "]." << std::endl;
#endif
}

void JVM_GetBooleanAttributes0(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *file = (InstanceOop *)_stack.front();	_stack.pop_front();

	Oop *result;
	file->get_field_value(JFILE L":path:Ljava/lang/String;", &result);
	const char *path = wstring_to_utf8(java_lang_string::stringOop_to_wstring((InstanceOop *)result)).c_str();

	// get file msg
	struct stat64 stat;
	if (stat64(path, &stat) == -1) {
		assert(false);
	}

	int mode = 0;
	// Exists:
	mode |= 1;
	// Regular:
	if ((stat.st_mode & S_IFMT) == S_IFREG)	mode |= 2;
	// Dir:
	if ((stat.st_mode & S_IFMT) == S_IFDIR)	mode |= 4;

	_stack.push_back(new IntOop(mode));

#ifdef DEBUG
	std::wcout << "(DEBUG) file: [" << path << "]'s attribute: [" << std::hex << mode << "]." << std::endl;
#endif
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


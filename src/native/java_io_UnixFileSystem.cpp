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

	std::wstring path = java_lang_string::stringOop_to_wstring(str);

	// 注意：此地略有玄机。在 jdk 源码中，FilePermission.init() 中，会向文件 xx/xxxx/xx/ 的后边加上一个 "-" 表示递归文件夹。还有一个 "*".
	bool has_final_char = false;
	wchar_t final_char = path[path.size() - 1];
	if (final_char == L'*' || final_char == L'-') {
		path.erase(path.end() - 1);		// remove the final char.
		has_final_char = true;
	}

	wstring canonical_path = boost::filesystem::canonical(path).wstring();

	if (has_final_char) {
		canonical_path += final_char;
	}

	_stack.push_back(java_lang_string::intern(canonical_path));

#ifdef DEBUG
	std::wcout << "(DEBUG) canonical path of [" << java_lang_string::stringOop_to_wstring(str) << "] is [" << java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.back()) << "]." << std::endl;
#endif
}

void JVM_GetBooleanAttributes0(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *file = (InstanceOop *)_stack.front();	_stack.pop_front();

	Oop *result;
	file->get_field_value(JFILE L":path:Ljava/lang/String;", &result);
//	std::wcout.imbue(std::locale(""));			// IMPORTANT!!!!! 如果不指定...... 用 下边 wcout 输出，是正确的......但是 path 真正的值其实只有一个 "x"......
	std::wcout << wstring_to_utf8(java_lang_string::stringOop_to_wstring((InstanceOop *)result)).c_str() << std::endl;	// delete

	// 注：通过 wstring_to_utf8 转成的 string 是没有问题的。有问题的是对这个 string 直接求 c_str() 得到 char*。
	// 这个提取 c_str() 的过程，是可能会出现各种诡异的状况的。
	// 非常好的解决方法，就是直接复制一份 string 出来，就没问题了。
	std::string backup_str = wstring_to_utf8(java_lang_string::stringOop_to_wstring((InstanceOop *)result));

//	const char *path = .c_str();
//	const char *path = narrow(java_lang_string::stringOop_to_wstring((InstanceOop *)result)).c_str();

//	std::wcout << "(((" << backup_str.c_str() << ")))" << std::endl;	// 竟然复制一份 转换成的 string 就好了？？？

	const char *path = backup_str.c_str();

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


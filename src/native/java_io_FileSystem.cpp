/*
 * java_io_FileSystem.cpp
 *
 *  Created on: 2017年12月7日
 *      Author: zhengxiaolin
 */

#include "native/java_io_FileSystem.hpp"
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
    {L"getLength:(" FLE ")J",					(void *)&JVM_GetLength},
};

void JVM_GetLength(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *file = (InstanceOop *)_stack.front();	_stack.pop_front();

	Oop *result;
	file->get_field_value(JFILE L":path:Ljava/lang/String;", &result);
#ifdef DEBUG
	sync_wcout{} << wstring_to_utf8(java_lang_string::stringOop_to_wstring((InstanceOop *)result)).c_str() << std::endl;	// delete
#endif
	std::string backup_str = wstring_to_utf8(java_lang_string::stringOop_to_wstring((InstanceOop *)result));

	const char *path = backup_str.c_str();

	// get file msg
	struct stat64 stat;
	if (stat64(path, &stat) == -1) {
		assert(false);
	}

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) file: [" << path << "]'s length: [" << stat.st_size << "]." << std::endl;
#endif

	_stack.push_back(new LongOop(stat.st_size));
}





// 返回 fnPtr.
void *java_io_fileSystem_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

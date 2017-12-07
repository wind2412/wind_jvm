/*
 * java_lang_Runtime.cpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Runtime.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "utils/os.hpp"

static unordered_map<wstring, void*> methods = {
    {L"availableProcessors:()I",				(void *)&JVM_AvailableProcessors},
};

// this method, I simplified it to just run the `run()` method.
void JVM_AvailableProcessors(list<Oop*>& _stack)
{
	_stack.push_back(new IntOop(get_cpu_nums()));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) now this machine has [" << ((IntOop *)_stack.back())->value << "] cpus." << std::endl;
#endif
}

// 返回 fnPtr.
void *java_lang_runtime_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


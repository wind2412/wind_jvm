/*
 * sun_misc_URLClassPath.cpp
 *
 *  Created on: 2017年12月3日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_URLClassPath.hpp"

#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"getLookupCacheURLs:(" JCL ")[Ljava/net/URL;",				(void *)&JVM_GetLookupCacheURLs},
};

void JVM_GetLookupCacheURLs(list<Oop *> & _stack){		// static
	_stack.push_back(nullptr);
}

void *sun_misc_urlClassPath_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

/*
 * test_native_find.cpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Object.hpp"
#include <string>

using std::string;

int main()
{
	assert(java_lang_object_search_method("wait") != nullptr);
}

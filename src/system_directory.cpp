/*
 * system_directory.cpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#include "system_directory.hpp"
#include "runtime/klass.hpp"

unordered_map<wstring, shared_ptr<InstanceKlass>> system_classmap;

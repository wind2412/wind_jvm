/*
 * system_directory.cpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#include "system_directory.hpp"
#include "runtime/klass.hpp"

Lock system_classmap_lock;

unordered_map<wstring, Klass *> system_classmap;

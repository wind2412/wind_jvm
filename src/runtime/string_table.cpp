/*
 * string_table.cpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#include "runtime/string_table.hpp"

unordered_set<shared_ptr<wstring>, wstring_hash, shared_wstring_equal_to> string_table;

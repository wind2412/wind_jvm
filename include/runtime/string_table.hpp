/*
 * string_table.hpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_STRING_TABLE_HPP_
#define INCLUDE_RUNTIME_STRING_TABLE_HPP_

#include <unordered_set>
#include <memory>
#include <string>

using std::wstring;
using std::shared_ptr;
using std::unordered_set;

struct wstring_hash	// Hash 用于决定此 T 变量放到哪个桶中，equator 才是判断两个 T 相不相同的重要因素！！
{
	size_t operator()(const shared_ptr<wstring> & s) const noexcept
	{
		return std::hash<std::wstring>{}(*s);
	}
};

struct shared_wstring_equal_to
{
	bool operator() (const shared_ptr<wstring> & lhs, const shared_ptr<wstring> & rhs) const
	{
		return *lhs == *rhs;
	}
};

extern unordered_set<shared_ptr<wstring>, wstring_hash, shared_wstring_equal_to> string_table;		// only save **RAW** unicode string, but not oop !

#endif /* INCLUDE_RUNTIME_STRING_TABLE_HPP_ */

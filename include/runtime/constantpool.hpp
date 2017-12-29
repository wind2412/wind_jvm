/*
 * constantpool.hpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_CONSTANTPOOL_HPP_
#define INCLUDE_RUNTIME_CONSTANTPOOL_HPP_

#include "class_parser.hpp"
#include <vector>
#include <utility>
#include <boost/any.hpp>
#include <cassert>
#include <memory>

using std::vector;
using std::pair;
using std::wstring;
using std::shared_ptr;

class Klass;
class InstanceKlass;
class GC;

class rt_constant_pool {	// runtime constant pool
	friend GC;
private:
	InstanceKlass *this_class;
	int this_class_index;
	cp_info **bufs;
	vector<pair<int, boost::any>> pool;
	ClassLoader *loader;
private:
	Klass *if_didnt_load_then_load(ClassLoader *loader, const wstring & name);
public:
	explicit rt_constant_pool(InstanceKlass *this_class, ClassLoader *loader, ClassFile *cf)
			: this_class(this_class), loader(loader), this_class_index(cf->this_class), bufs(cf->constant_pool), pool(cf->constant_pool_count-1, std::make_pair(0, boost::any())) {}	// 别忘了 -1 啊！！！！		// bufs 前边加上 const 竟然会报错 ???
private:
	const pair<int, boost::any> & if_didnt_parse_then_parse(int index);
public:
	pair<int, boost::any> operator[] (int index) {
		return if_didnt_parse_then_parse(index);
	}
public:
	void print_debug();
};




#endif /* INCLUDE_RUNTIME_CONSTANTPOOL_HPP_ */

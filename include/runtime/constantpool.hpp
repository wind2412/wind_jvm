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
using std::shared_ptr;
using std::wstring;

class Klass;
class InstanceKlass;

class rt_constant_pool {	// runtime constant pool
private:
	shared_ptr<InstanceKlass> this_class;
	int this_class_index;
	cp_info **bufs;
	vector<pair<int, boost::any>> pool;
	ClassLoader *loader;
private:
	shared_ptr<Klass> if_didnt_load_then_load(ClassLoader *loader, const wstring & name);		// 有可能 load 到 数组类......
public:
	explicit rt_constant_pool(shared_ptr<InstanceKlass> this_class, ClassLoader *loader, shared_ptr<ClassFile> cf)
			: this_class(this_class), loader(loader), this_class_index(cf->this_class), bufs(cf->constant_pool), pool(cf->constant_pool_count-1, std::make_pair(0, boost::any())) {}	// 别忘了 -1 啊！！！！		// bufs 前边加上 const 竟然会报错 ???
public:
	pair<int, boost::any> if_didnt_parse_then_parse(int index);		// 把 cp_info static 常量池中的元素(符号引用) 解析成为 引用。
	int type(int index) {
		return pool[index].first;
	}
	boost::any at(int index) {
		assert(index >= 0 && index < pool.size());
		return pool[index].second;
	}
public:
	void print_debug();
};




#endif /* INCLUDE_RUNTIME_CONSTANTPOOL_HPP_ */

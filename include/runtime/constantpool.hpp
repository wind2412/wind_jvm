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
	vector<pair<int, boost::any>> pool;
	ClassLoader *loader;
private:
	shared_ptr<Klass> if_didnt_load_then_load(ClassLoader *loader, const wstring & name);		// 有可能 load 到 数组类......
public:
	explicit rt_constant_pool(shared_ptr<InstanceKlass> this_class, ClassLoader *loader, const ClassFile & cf);		// bufs 前边加上 const 竟然会报错 ???
public:
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

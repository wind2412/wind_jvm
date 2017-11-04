/*
 * constantpool.hpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_CONSTANTPOOL_HPP_
#define INCLUDE_RUNTIME_CONSTANTPOOL_HPP_

#include <class_parser.hpp>
#include <vector>
#include <utility>
#include <boost/any.hpp>
#include <cassert>

using std::vector;
using std::pair;


class Klass;

class rt_constant_pool {	// runtime constant pool
private:
	vector<pair<int, boost::any>> pool;
	ClassLoader *loader;
public:
	explicit rt_constant_pool(Klass *this_class, int this_class_index, cp_info **bufs, int length, ClassLoader *loader);		// bufs 前边加上 const 竟然会报错 ???
	int type(int index) {
		return pool[index].first;
	}
	boost::any at(int index) {
		assert(index >= 0 && index < pool.size());
		return pool[index].second;
	}
};




#endif /* INCLUDE_RUNTIME_CONSTANTPOOL_HPP_ */

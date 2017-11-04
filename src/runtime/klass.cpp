/*
 * klass.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "runtime/klass.hpp"

InstanceKlass::InstanceKlass(const ClassFile & cf, ClassLoader *loader) : rt_pool(this, cf.this_class, cf.constant_pool, cf.constant_pool_count, loader)		// first constant_pool
{
	// this_class
	assert(this->rt_pool.type(cf.this_class-1) == CONSTANT_Class);
	this->name = *boost::any_cast<wstring *>(this->rt_pool.at(boost::any_cast<int>(this->rt_pool.at(cf.this_class-1))));
	// super_class
	if (cf.super_class == 0) {	// this class = java/lang/Object
		this->parent = nullptr;
	} else {			// base class
		// TODO: 递归先解析...
		// TODO: 貌似没对 java.lang.Object 父类进行处理。比如 wait 方法等等...
		// TODO: Reference Class ... and so on

	}
	assert(this->rt_pool.type(cf.super_class-1) == CONSTANT_Class);
//	this->parent = boost::any_cast<Klass *>(this->rt_pool.at(this->))

}



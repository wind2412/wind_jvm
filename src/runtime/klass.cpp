/*
 * klass.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "runtime/klass.hpp"
#include "runtime/field.hpp"
#include "classloader.hpp"
#include <utility>
#include <cstring>

using std::make_pair;
using std::make_shared;

void InstanceKlass::parse_methods(const ClassFile & cf)
{
	for(int i = 0; i < cf.methods_count; i ++) {
		this->methods.insert(make_pair(i, new Method(this, cf)));
	}
}

void InstanceKlass::parse_fields(const ClassFile & cf)
{
	// set up Runtime Field_info to transfer Non-Dynamic field_info
	for (int i = 0; i < cf.fields_count; i ++) {
		shared_ptr<Field_info> metaField = make_shared<Field_info>(cf.fields[i], cf.constant_pool);
		if((cf.fields[i].access_flags & 0x08) != 0) {	// static field
			this->static_fields_layout.insert(make_pair(i, make_pair(total_static_fields_bytes, metaField)));
			total_static_fields_bytes += metaField->get_value_size();	// offset +++
		} else {		// non-static field
			this->fields_layout.insert(make_pair(i, make_pair(total_static_fields_bytes, metaField)));
			total_non_static_fields_bytes += metaField->get_value_size();
		}
	}

	// alloc to save value of STATIC fields. non-statics are in oop.
	this->static_fields = new uint8_t[total_static_fields_bytes];
	memset(this->static_fields, 0, total_static_fields_bytes);	// bzero!!
}

InstanceKlass::InstanceKlass(const ClassFile & cf, ClassLoader *loader) : loader(loader)
{
	// become Runtime methods
	parse_methods(cf);
	// become Runtime fields
	parse_fields(cf);

	// super_class
	if (cf.super_class == 0) {	// this class = java/lang/Object		// TODO: java.lang.Object 怎么办？是个接口？？
		this->parent = nullptr;
	} else {			// base class
		// TODO: 递归先解析...
		// TODO: 貌似没对 java.lang.Object 父类进行处理。比如 wait 方法等等...
		// TODO: Reference Class ... and so on
		if (loader == nullptr) {	// bootstrap classloader do this

		}
	}
	// this_class

	// TODO: Runtime constant pool and remove Non-Dynamic cp_pool.
}



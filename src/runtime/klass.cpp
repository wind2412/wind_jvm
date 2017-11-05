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

void InstanceKlass::parse_interfaces(const ClassFile & cf, ClassLoader *loader)	// interface should also be made by the InstanceKlass!!
{
	for(int i = 0; i < cf.interfaces_count; i ++) {
		// get interface name
		assert(cf.constant_pool[cf.interfaces[i]-1]->tag == CONSTANT_Class);
		wstring interface_name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.interfaces[i]-1])->index-1])->convert_to_Unicode();
		shared_ptr<InstanceKlass> interface;
		if (loader == nullptr) {
			interface = BootStrapClassLoader::get_bootstrap().loadClass(interface_name);
		} else {
			interface = MyClassLoader::get_loader().loadClass(interface_name);
		}
		assert(interface != nullptr);
		this->interfaces.insert(make_pair(i, interface));
	}
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
		// TODO: java.lang.Class: mirror!!!!
		// TODO: 貌似没对 java.lang.Object 父类进行处理。比如 wait 方法等等...
		// TODO: ReferenceKlass......
		// TODO: Inner Class!!
		assert(cf.constant_pool[cf.super_class-1]->tag == CONSTANT_Class);
		wstring super_name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.super_class-1])->index-1])->convert_to_Unicode();
		if (loader == nullptr) {	// bootstrap classloader do this
			this->parent = BootStrapClassLoader::get_bootstrap().loadClass(super_name);
		} else {		// my classloader do this
			this->parent = MyClassLoader::get_loader().loadClass(super_name);
		}

		if (this->parent != nullptr) {
			this->next_sibling = this->parent->child;		// set this's elder brother
			this->parent->child = this;					// set parent's newest child
		}
	}

	// this_class
	assert(cf.constant_pool[cf.this_class-1]->tag == CONSTANT_Class);
	this->name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.this_class-1])->index-1])->convert_to_Unicode();
	this->access_flags = cf.access_flags;
	cur = Loaded;

	// become Runtime interfaces
	parse_interfaces(cf, loader);

	// TODO: enum status, Loaded, Parsed...
	// TODO: Runtime constant pool and remove Non-Dynamic cp_pool.
}



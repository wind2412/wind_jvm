/*
 * klass.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "runtime/klass.hpp"
#include "runtime/field.hpp"
#include "classloader.hpp"
#include "runtime/constantpool.hpp"
#include <utility>
#include <cstring>
#include <sstream>

using std::make_pair;
using std::make_shared;
using std::wstringstream;

void InstanceKlass::parse_methods(const ClassFile & cf)
{
	wstringstream ss;
	for(int i = 0; i < cf.methods_count; i ++) {
		shared_ptr<Method> method = make_shared<Method>(this, cf.methods[i], cf.constant_pool);
		ss << method->get_name() << L":" << method->get_descriptor();
		this->methods.insert(make_pair(ss.str(), method));	// add into
		ss.str(L"");		// make empty
	}
#ifdef DEBUG
	std::wcout << "===--------------- (" << this->get_name() << ") Debug Runtime MethodPool ---------------===" << std::endl;
	std::cout << "methods: total " << this->methods.size() << std::endl;
	int counter = 0;
	for (auto iter : this->methods) {
		std::wcout << "  #" << counter++ << "  " << iter.first << std::endl;
	}
	std::cout << "===---------------------------------------------------------===" << std::endl;
#endif
}

void InstanceKlass::parse_fields(const ClassFile & cf)
{
	wstringstream ss;
	// set up Runtime Field_info to transfer Non-Dynamic field_info
	for (int i = 0; i < cf.fields_count; i ++) {
		shared_ptr<Field_info> metaField = make_shared<Field_info>(cf.fields[i], cf.constant_pool);
		ss << metaField->get_name() << L":" << metaField->get_descriptor();
		if((cf.fields[i].access_flags & 0x08) != 0) {	// static field
			this->static_fields_layout.insert(make_pair(ss.str(), make_pair(total_static_fields_bytes, metaField)));
			total_static_fields_bytes += metaField->get_value_size();	// offset +++
		} else {		// non-static field
			this->fields_layout.insert(make_pair(ss.str(), make_pair(total_static_fields_bytes, metaField)));
			total_non_static_fields_bytes += metaField->get_value_size();
		}
		ss.str(L"");
	}

	// alloc to save value of STATIC fields. non-statics are in oop.
	this->static_fields = new uint8_t[total_static_fields_bytes];
	memset(this->static_fields, 0, total_static_fields_bytes);	// bzero!!
#ifdef DEBUG
	std::wcout << "===--------------- (" << this->get_name() << ") Debug Runtime FieldPool ---------------===" << std::endl;
	std::cout << "static Field: " << this->static_fields_layout.size() << "; non-static Field: " << this->fields_layout.size() << std::endl;
	if (this->fields_layout.size() != 0)		std::cout << "non-static as below:" << std::endl;
	int counter = 0;
	for (auto iter : this->fields_layout) {
		std::wcout << "  #" << counter++ << "  name: " << iter.first << ", offset: " << iter.second.first << ", size: " << iter.second.second->get_value_size() << std::endl;
	}
	counter = 0;
	if (this->static_fields_layout.size() != 0)	std::cout << "static as below:" << std::endl;
	for (auto iter : this->static_fields_layout) {
		std::wcout << "  #" << counter++ << "  name: " << iter.first << ", offset: " << iter.second.first << ", size: " << iter.second.second->get_value_size() << std::endl;
	}
	std::cout << "===--------------------------------------------------------===" << std::endl;
#endif
}

void InstanceKlass::parse_superclass(const ClassFile & cf, ClassLoader *loader)
{
	if (cf.super_class == 0) {	// this class = java/lang/Object		// TODO: java.lang.Object 怎么办？是个接口？？
		this->parent = nullptr;
	} else {			// base class
		assert(cf.constant_pool[cf.super_class-1]->tag == CONSTANT_Class);
		wstring super_name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.super_class-1])->index-1])->convert_to_Unicode();
		std::wcout << super_name << std::endl;
		if (loader == nullptr) {	// bootstrap classloader do this
			this->parent = BootStrapClassLoader::get_bootstrap().loadClass(super_name);
		} else {		// my classloader do this
			this->parent = loader->loadClass(super_name);
		}

		if (this->parent != nullptr) {
			this->next_sibling = this->parent->get_child();		// set this's elder brother	// note: this->parent->child can't pass the compile. because this->parent is okay, but parent->child is visiting Klass in the InstanceKlass. `Protected` is: InstanceKlass can visit [the Klass part] inside of the IntanceKlass object. But here is: InstanceKlass visit the Klass part inside of the InstanceKlass part(this->parent), but then visit the Klass outer class (parent->child). parent variable is inside the InstanceKlass, but point to an outer Klass not in the InstanceKlass. To solve it, only use setters and getters.
			this->parent->set_child(shared_ptr<InstanceKlass>(this, [](InstanceKlass*){}));			// set parent's newest child
			// above ↑ is a little hack. I don't know whether there is a side effect.
		}
	}
#ifdef DEBUG
	std::wcout << "===--------------- (" << this->get_name() << ") Debug SuperClass ---------------===" << std::endl;
	if (cf.super_class == 0) {
		std::cout << "this class is **java.lang.Object** class and doesn't have a superclass." << std::endl;
	} else {
		std::wcout << "superclass:  #" << cf.super_class << ", name: " << this->parent->get_name() << std::endl;
	}
	std::cout << "===-------------------------------------------------===" << std::endl;
#endif
}

void InstanceKlass::parse_interfaces(const ClassFile & cf, ClassLoader *loader)	// interface should also be made by the InstanceKlass!!
{
	for(int i = 0; i < cf.interfaces_count; i ++) {
		// get interface name
		assert(cf.constant_pool[cf.interfaces[i]-1]->tag == CONSTANT_Class);
		wstring interface_name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.interfaces[i]-1])->index-1])->convert_to_Unicode();
		shared_ptr<InstanceKlass> interface;
		if (loader == nullptr) {
			std::cout << "wrong..." << std::endl;	//delete
			interface = BootStrapClassLoader::get_bootstrap().loadClass(interface_name);
		} else {
			interface = loader->loadClass(interface_name);
			assert(interface != nullptr);
		}
		assert(interface != nullptr);
		this->interfaces.insert(make_pair(interface_name, interface));
	}
#ifdef DEBUG
	std::wcout << "===--------------- (" << this->get_name() << ") Debug Runtime InterfacePool ---------------===" << std::endl;
	std::cout << "interfaces: total " << this->interfaces.size() << std::endl;
	int counter = 0;
	for (auto iter : this->interfaces) {
		std::wcout << "  #" << counter++ << "  name: " << iter.first << std::endl;
	}
	std::cout << "===------------------------------------------------------------===" << std::endl;
#endif
}

void InstanceKlass::parse_constantpool(const ClassFile & cf, ClassLoader *loader)
{
	shared_ptr<InstanceKlass> this_class(this, [](InstanceKlass*){});
	this->rt_pool = make_shared<rt_constant_pool>(this_class, loader, cf);
}

InstanceKlass::InstanceKlass(const ClassFile & cf, ClassLoader *loader) : loader(loader), Klass()
{
	// this_class (only name)
	assert(cf.constant_pool[cf.this_class-1]->tag == CONSTANT_Class);
	this->name = ((CONSTANT_Utf8_info *)cf.constant_pool[((CONSTANT_CS_info *)cf.constant_pool[cf.this_class-1])->index-1])->convert_to_Unicode();

	// become Runtime methods
	parse_methods(cf);
	// become Runtime fields
	parse_fields(cf);
	// super_class
	parse_superclass(cf, loader);
	// this_class
	this->access_flags = cf.access_flags;
	cur = Loaded;
	// become Runtime interfaces
	parse_interfaces(cf, loader);
	// become Runtime constant pool
//	parse_constantpool(cf, loader);

	// TODO: enum status, Loaded, Parsed...
	// TODO: Runtime constant pool and remove Non-Dynamic cp_pool.
	// TODO: annotations...
	// TODO: java.lang.Class: mirror!!!!
	// TODO: 貌似没对 java.lang.Object 父类进行处理。比如 wait 方法等等...
	// TODO: ReferenceKlass......
	// TODO: Inner Class!!
	// TODO: 补全 oop 的 Fields.

#ifdef DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
}



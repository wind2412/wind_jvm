/*
 * field.hpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_FIELD_HPP_
#define INCLUDE_RUNTIME_FIELD_HPP_

#include "utils.hpp"

/**
 * all non-static fields [values]. save in oop object.
 */
class Fields {
private:
	InstanceKlass *klass;		// search [static field / non-static field layout] via this klass
	uint8_t *fields;				// non-static field <real values>
public:
	Fields(InstanceKlass *klass) : klass(klass) {		// 通过 运行时常量池来 parse ！ 不着急。最后再弄。
//		for (int i = 0; i < klass->cf->)
	}
};

/**
 * all non-static/static fields [properties in field_info]. save in klass.cpp.
 * all Field in constant_pool is only belong to *this* class, the same as Method.
 */
class Field_info {
private:
	// field basic
	u2 access_flags;
	wstring name;			// variable name
	wstring descriptor;		// type descripror: I, [I, java.lang.String etc.
	int value_size;			// **this field's size**: parse descriptor to get it. See: Java SE 8 Specification $4.3.2

	// attributes
	u2 attributes_count;
	attribute_info **attributes;
public:
	explicit Field_info(const field_info & fi, cp_info **constant_pool) {
		this->access_flags = fi.access_flags;
		assert(constant_pool[fi.name_index-1]->tag == CONSTANT_Utf8 && constant_pool[fi.descriptor_index-1]->tag == CONSTANT_Utf8);
		this->name = ((CONSTANT_Utf8_info *)constant_pool[fi.name_index-1])->convert_to_Unicode();
		this->descriptor = ((CONSTANT_Utf8_info *)constant_pool[fi.name_index-1])->convert_to_Unicode();
		this->attributes_count = fi.attributes_count;
		this->attributes = fi.attributes;
		value_size = parse_field_descriptor(descriptor);
	}
	const wstring & get_name() { return name; }
	const wstring & get_descriptor() { return descriptor; }
	int get_value_size() { return value_size; }
	// TODO: attributes 最后再补。
	// TODO: 常量池要变成动态的。在此 class 变成 klass 之后，再做吧。
};

#endif /* INCLUDE_RUNTIME_FIELD_HPP_ */

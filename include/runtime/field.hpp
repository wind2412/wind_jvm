/*
 * field.hpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_FIELD_HPP_
#define INCLUDE_RUNTIME_FIELD_HPP_

#include "utils.hpp"
#include "class_parser.hpp"
#include <vector>
#include <string>
#include <cassert>

using std::wstring;
using std::vector;

class InstanceKlass;

/**
 * all non-static fields [values]. save in oop object.
// */
//class Fields {
//private:
//	InstanceKlass *klass;		// search [static field / non-static field layout] via this klass
//	uint8_t *fields;				// non-static field <real values>
//public:
//	Fields(InstanceKlass *klass) : klass(klass) {		// 通过 运行时常量池来 parse ！ 不着急。最后再弄。
////		for (int i = 0; i < klass->cf->)
//	}
//};

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
	// 0, 6, 7, 13, 14, 15, 18, 19	// synthetic, deprecated 两者都不需要。并没有保存任何信息。

	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	ConstantValue_attribute *constant_value = nullptr;	// only one
	u2 signature_index;
	// TODO: support Annotations


public:
	explicit Field_info(field_info & fi, cp_info **constant_pool);
	const wstring & get_name() { return name; }
	const wstring & get_descriptor() { return descriptor; }
	int get_value_size() { return value_size; }
	bool is_static() { return (access_flags & ACC_STATIC) == ACC_STATIC; }
	void print() { std::wcout << name << ":" << descriptor; }
	// TODO: attributes 最后再补。
	// TODO: 常量池要变成动态的。在此 class 变成 klass 之后，再做吧。
	~Field_info () {
		for (int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes;
	}
};

#endif /* INCLUDE_RUNTIME_FIELD_HPP_ */

/*
 * field.hpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_FIELD_HPP_
#define INCLUDE_RUNTIME_FIELD_HPP_

#include "utils/utils.hpp"
#include "class_parser.hpp"
#include <vector>
#include <string>
#include <cassert>
#include <memory>
#include <boost/any.hpp>
#include "runtime/klass.hpp"
#include "annotation.hpp"

using std::wstring;
using std::vector;
using std::shared_ptr;

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
public:
	enum FieldState { NotParsed, Parsing, Parsed };
private:
	FieldState state = NotParsed;

	// field basic
	shared_ptr<InstanceKlass> klass;		// the klass This Field_info is in.

	Type type;
	shared_ptr<Klass> true_type;			// the klass which is This Field_info 's type ! which is the `descriptor`...

	u2 access_flags;
	wstring name;			// variable name
	wstring descriptor;		// type descripror: I, [I, java.lang.String etc.

	// attributes
	// 0, 6, 7, 13, 14, 15, 18, 19	// synthetic, deprecated, RuntimeInvisible(Type)Annotation 这几者都不需要。并没有保存任何信息。

	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	// 其实我设置 RuntimeVisibleAnnotation 内部是一个 ParameterAnnotation.
	Parameter_annotations_t *rva = nullptr;

	u2 num_RuntimeVisibleTypeAnnotations;
	TypeAnnotation *rvta = nullptr;

	ConstantValue_attribute *constant_value = nullptr;	// only one
	u2 signature_index = 0;
	// TODO: support Annotations


public:
	explicit Field_info(shared_ptr<InstanceKlass> klass, field_info & fi, cp_info **constant_pool);
	FieldState get_state() { return state; }
	void set_state(FieldState s) { state = s; }
	const wstring & get_name() { return name; }						// TODO: 重要！！......忘了给 父类的 fields 留空间了......子类肯定要继承的啊！！！QAQQAQQAQ
	const wstring & get_descriptor() { return descriptor; }
	shared_ptr<InstanceKlass> get_klass() { return klass; }
	Type get_type() { return type; }
	shared_ptr<Klass> get_type_klass() { assert(get_state() == Parsed); return true_type; }	// small lock to keep safe.
	void if_didnt_parse_then_parse();			// **VERY IMPORTANT**!!!
public:
	bool is_static() { return (access_flags & ACC_STATIC) == ACC_STATIC; }
	bool is_final() { return (access_flags & ACC_FINAL) == ACC_FINAL; }
public:
	ConstantValue_attribute *get_constant_value() { return constant_value; }
	wstring parse_signature();
public:
	void print() { std::wcout << name << ":" << descriptor; }
	// TODO: attributes 最后再补。
	// TODO: 常量池要变成动态的。在此 class 变成 klass 之后，再做吧。
	~Field_info ();
};

#endif /* INCLUDE_RUNTIME_FIELD_HPP_ */

/*
 * annotation.hpp
 *
 *  Created on: 2017年11月23日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_ANNOTATION_HPP_
#define INCLUDE_RUNTIME_ANNOTATION_HPP_

#include <string>
#include <cassert>
#include "class_parser.hpp"


using std::wstring;

// RuntimeVisibleAnnotation
struct Value_t {};

struct Const_value_t : public Value_t {
	wstring const_value;
	Const_value_t(cp_info **constant_pool, const const_value_t & v);
};

struct Enum_const_value_t : public Value_t {
	wstring type_name;
	wstring const_name;
	Enum_const_value_t(cp_info **constant_pool, const enum_const_value_t & v);
};

struct Class_info_t : public Value_t {
	wstring class_info;
	Class_info_t(cp_info **constant_pool, const class_info_t & v);
};

struct Element_value {
	int tag;
	Value_t *value = nullptr;	// [1]
	Element_value(cp_info **constant_pool, const element_value & v);
	~Element_value();
	CodeStub stub;
};

struct Annotation : public Value_t {
	wstring type;
	int num_element_value_pairs;
	struct Element_value_pairs_t {
		wstring element_name;
		Element_value value;

		Element_value_pairs_t(cp_info **constant_pool, const annotation::element_value_pairs_t & v);
	} *element_value_pairs = nullptr;		// [num_element_value_pairs]
	CodeStub stub;

	Annotation(cp_info **constant_pool, const annotation & v);
	~Annotation();
public:
	bool match_annotation_name(const wstring & name) { return name == type; }
};

struct Array_value_t : public Value_t {
	int num_values;
	Element_value *values = nullptr;		// [num_values]

	Array_value_t(cp_info **constant_pool, const array_value_t & v);
	~Array_value_t();
};


// RuntimeVisibleParameterAnnotation
struct Parameter_annotations_t {	// extract from Runtime_XXX_Annotations_attributes
	u2 num_annotations;
	Annotation *annotations = nullptr;	// [num_annotations]
	CodeStub stub;

	Parameter_annotations_t(cp_info **constant_pool, const parameter_annotations_t & v);
	~Parameter_annotations_t();
public:
	bool has_annotation_name(const wstring & name) {
		for (int i = 0; i < num_annotations; i ++) {
			if (annotations[i].match_annotation_name(name)) {
				return true;
			}
		}
		return false;
	}
};


// RuntimeVisibleTypeAnnotation
struct TypeAnnotation {
	// basic
	u1 target_type;
	type_annotation::target_info_t *target_info = nullptr;	// [1]
	type_annotation::type_path target_path;
	Annotation *anno = nullptr;			// [1]	// only this CHANGED.

	TypeAnnotation(cp_info **constant_pool, type_annotation & v);
	~TypeAnnotation();
public:
	bool has_annotation_name(const wstring & name) {
		return anno->match_annotation_name(name);
	}
};

#endif /* INCLUDE_RUNTIME_ANNOTATION_HPP_ */

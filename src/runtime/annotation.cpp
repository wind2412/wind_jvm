/*
 * annotation.cpp
 *
 *  Created on: 2017年11月23日
 *      Author: zhengxiaolin
 */

#include "runtime/annotation.hpp"
#include "utils/utils.hpp"

// 注意：因为设置了各种结构体全带构造函数，为了延迟构造，因此使用了 malloc + placement new 的方式。

// RuntimeVisibleAnnotation
Const_value_t::Const_value_t(cp_info **constant_pool, const const_value_t & v) : const_value(((CONSTANT_Utf8_info *)constant_pool[v.const_value_index-1])->convert_to_Unicode()) {
	assert(constant_pool[v.const_value_index-1]->tag == CONSTANT_Utf8);
}

Enum_const_value_t::Enum_const_value_t(cp_info **constant_pool, const enum_const_value_t & v)
		: type_name(((CONSTANT_Utf8_info *)constant_pool[v.type_name_index-1])->convert_to_Unicode()),
		  const_name(((CONSTANT_Utf8_info *)constant_pool[v.const_name_index-1])->convert_to_Unicode()) {
	assert(constant_pool[v.type_name_index-1]->tag == CONSTANT_Utf8);
	assert(constant_pool[v.const_name_index-1]->tag == CONSTANT_Utf8);
}

Class_info_t::Class_info_t(cp_info **constant_pool, const class_info_t & v) : class_info(((CONSTANT_Utf8_info *)constant_pool[v.class_info_index-1])->convert_to_Unicode()) {
	assert(constant_pool[v.class_info_index-1]->tag == CONSTANT_Utf8);
}

Element_value::Element_value(cp_info **constant_pool, const element_value & v) : tag(v.tag), stub(v.stub) {
	switch ((char)tag) {
		case 'B':
		case 'C':
		case 'D':
		case 'F':
		case 'I':
		case 'J':
		case 'S':
		case 'Z':
		case 's':{
			value = new Const_value_t(constant_pool, *(const_value_t *)v.value);
			break;
		}
		case 'e':{
			value = new Enum_const_value_t(constant_pool, *(enum_const_value_t *)v.value);
			break;
		}
		case 'c':{
			value = new Class_info_t(constant_pool, *(class_info_t *)v.value);
			break;
		}
		case '@':{
			value = new Annotation(constant_pool, *(annotation *)v.value);
			break;
		}
		case '[':{
			value = new Array_value_t(constant_pool, *(array_value_t *)v.value);
			break;
		}
		default:{
			std::wcerr << "can't get here. in element_value." << std::endl;
			assert(false);
		}
	}

}
Element_value::~Element_value() { delete value; };


Annotation::Element_value_pairs_t::Element_value_pairs_t(cp_info **constant_pool, const annotation::element_value_pairs_t & v)
		: element_name(((CONSTANT_Utf8_info *)constant_pool[v.element_name_index-1])->convert_to_Unicode()),
		  value(constant_pool, v.value){
	assert(constant_pool[v.element_name_index-1]->tag == CONSTANT_Utf8);
}

Annotation::Annotation(cp_info **constant_pool, const annotation & v)
		 : type(((CONSTANT_Utf8_info *)constant_pool[v.type_index-1])->convert_to_Unicode()), stub(v.stub),
		   num_element_value_pairs(v.num_element_value_pairs), element_value_pairs((Element_value_pairs_t *)malloc(sizeof(Element_value_pairs_t) * num_element_value_pairs)/*::new Element_value_pairs_t[num_element_value_pairs] 报错。貌似 Ele..._t 类中必须重载 ::new 才行？？难道不是自动检测的吗...*/){		// TODO: 这里 new，但是并不在 GC 管辖范围内。甚至包括了 class_parser 的各种new......
	assert(constant_pool[v.type_index-1]->tag == CONSTANT_Utf8);
	std::wcout << "ohhhhh0" << std::endl;		// delete 分配了这么多......
	for (int i = 0; i < num_element_value_pairs; i ++) {
		constructor(&element_value_pairs[i], constant_pool, v.element_value_pairs[i]);
	}
}

Annotation::~Annotation() {
	std::wcout << "ohhhhh" << std::endl;			// delete 竟然没释放......
	for (int i = 0; i < num_element_value_pairs; i ++) {
		destructor(&element_value_pairs[i]);
	}
	free(element_value_pairs);
}

Array_value_t::Array_value_t(cp_info **constant_pool, const array_value_t & v) : num_values(v.num_values), values((Element_value *)malloc(sizeof(Element_value) * num_values)/* TODO: 理由同上。 ::new Element_value[num_values]*/) {
	for (int i = 0; i < num_values; i ++) {
		constructor(&values[i], constant_pool, v.values[i]);
	}
}

Array_value_t::~Array_value_t() {
	for (int i = 0; i < num_values; i ++) {
		destructor(&values[i]);
	}
	free(values);
}

// RuntimeVisibleParameterAnnotation
Parameter_annotations_t::Parameter_annotations_t(cp_info **constant_pool, const parameter_annotations_t & v)
					: stub(v.stub), num_annotations(v.num_annotations), annotations((Annotation *)malloc(sizeof(Annotation) * num_annotations)) {
	for (int i = 0; i < num_annotations; i ++) {
		constructor(&annotations[i], constant_pool, v.annotations[i]);
	}
}

Parameter_annotations_t::~Parameter_annotations_t() {
	for (int i = 0; i < num_annotations; i ++) {
		destructor(&annotations[i]);
	}
	free(annotations);
}

// RuntimeVisibleTypeAnnotation
TypeAnnotation::TypeAnnotation(cp_info **constant_pool, type_annotation & v) : target_type(v.target_type), target_info(v.target_info), target_path(v.target_path), anno((Annotation *)malloc(sizeof(Annotation))) {
	v.target_info = nullptr;		// 移动语义！直接抢过来。
	v.target_path.path = nullptr;
	constructor(anno, constant_pool, *v.anno);
}

TypeAnnotation::~TypeAnnotation() {
	destructor(anno);
	free(anno);
	delete target_info;	// 这是个指针，没法自动释放。
	// target_path 会自动执行写好的析构函数。
}

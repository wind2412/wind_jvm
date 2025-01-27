/*
 * field.cpp
 *
 *  Created on: 2017年11月10日
 *      Author: zhengxiaolin
 */

#include "runtime/field.hpp"
#include "classloader.hpp"
#include "utils/utils.hpp"
#include "runtime/constantpool.hpp"
#include "utils/synchronize_wcout.hpp"

Lock & Field_Pool::field_pool_lock(){
	static Lock field_pool_lock;
	return field_pool_lock;
}
list<Field_info *> & Field_Pool::field_pool() {
	static list<Field_info *> field_pool;		// 存放所有的对象，以备日后的 delete。
	return field_pool;
}
void Field_Pool::put(Field_info *field) {
	LockGuard lg(field_pool_lock());
	field_pool().push_back(field);
}
void Field_Pool::cleanup() {
	LockGuard lg(field_pool_lock());
	for (auto iter : field_pool()) {
		delete iter;
	}
}

Field_info::Field_info(InstanceKlass *klass, field_info & fi, cp_info **constant_pool) : klass(klass) {	// must be 0, 6, 7, 13, 14, 15, 18, 19
	this->access_flags = fi.access_flags;
	assert(constant_pool[fi.name_index-1]->tag == CONSTANT_Utf8 && constant_pool[fi.descriptor_index-1]->tag == CONSTANT_Utf8);
	this->name = ((CONSTANT_Utf8_info *)constant_pool[fi.name_index-1])->convert_to_Unicode();
	this->descriptor = ((CONSTANT_Utf8_info *)constant_pool[fi.descriptor_index-1])->convert_to_Unicode();

	// move!!! important!!!
	this->attributes = fi.attributes;
	fi.attributes = nullptr;

	// set to 0!!! important!!!
	this->attributes_count = fi.attributes_count;
	fi.attributes_count = 0;

	// traverse Attributes
	for (int i = 0; i < this->attributes_count; i ++) {
		int attribute_tag = peek_attribute(this->attributes[i]->attribute_name_index, constant_pool);
		switch (attribute_tag) {
			case 0:{
				this->constant_value = (ConstantValue_attribute *)this->attributes[i];
				break;
			}
			case 7:{
				this->signature_index = ((Signature_attribute *)this->attributes[i])->signature_index;
				break;
			}
			case 14:{		// RuntimeVisibleAnnotation
				auto & enter = ((RuntimeVisibleAnnotations_attribute *)this->attributes[i])->parameter_annotations;
				this->rva = (Parameter_annotations_t *)malloc(sizeof(Parameter_annotations_t));
				constructor(this->rva, constant_pool, enter);
				break;
			}
			case 18:{		// RuntimeVisibleTypeAnnotation
				auto enter_ptr = ((RuntimeVisibleTypeAnnotations_attribute *)this->attributes[i]);
				this->num_RuntimeVisibleTypeAnnotations = enter_ptr->num_annotations;
				this->rvta = (TypeAnnotation *)malloc(sizeof(TypeAnnotation) * this->num_RuntimeVisibleTypeAnnotations);
				for (int pos = 0; pos < this->num_RuntimeVisibleTypeAnnotations; pos ++) {
					constructor(&this->rvta[pos], constant_pool, enter_ptr->annotations[pos]);
				}
				break;
			}
			case 6:
			case 13:
			case 15:
			case 19:{		// do nothing
				break;
			}
			default:{
				std::cerr << "attribute_tag == " << attribute_tag << std::endl;
				assert(false);
			}
		}
	}

}

void Field_info::if_didnt_parse_then_parse()
{
	if (this->state != NotParsed)	return;
	// parse descriptor and parse `InstanceKlass type of this Field (in descriptor)`.
	switch (descriptor[0]) {
		case L'Z':{
			type = Type::BOOLEAN;
			break;
		}
		case L'B':{
			type = Type::BYTE;
			break;
		}
		case L'C':{
			type = Type::CHAR;
			break;
		}
		case L'S':{
			type = Type::SHORT;
			break;
		}
		case L'I':{
			type = Type::INT;
			break;
		}
		case L'F':{
			type = Type::FLOAT;
			break;
		}
		case L'J':{
			type = Type::LONG;
			break;
		}
		case L'D':{
			type = Type::DOUBLE;
			break;
		}
		case L'L':{
			type = Type::OBJECT;
//			std::wcout << "wrong!!! " << descriptor.substr(1, descriptor.size() - 2) << std::endl;
			auto loader = this->klass->get_classloader();
			if (loader == nullptr) {
				this->true_type = BootStrapClassLoader::get_bootstrap().loadClass(descriptor.substr(1, descriptor.size() - 2));
			} else {
				this->true_type = loader->loadClass(descriptor.substr(1, descriptor.size() - 2));
			}
			assert(true_type != nullptr);
			break;
		}
		case L'[':{
			type = Type::ARRAY;
			auto loader = this->klass->get_classloader();
			if (loader == nullptr) {
				this->true_type = BootStrapClassLoader::get_bootstrap().loadClass(descriptor);
			} else {
				this->true_type = loader->loadClass(descriptor);
			}
			assert(true_type != nullptr);
			break;
		}
		default:{
			std::cerr << "can't get here!" << std::endl;
			assert(false);
		}
	}
	this->state = Parsed;
}

wstring Field_info::parse_signature() {
	if (signature_index == 0) return L"";
	auto _pair = (*klass->get_rtpool())[signature_index];
	assert(_pair.first == CONSTANT_Utf8);
	wstring signature = boost::any_cast<wstring>(_pair.second);
	assert(signature != L"");
	return signature;
}

Field_info::~Field_info () {
	for (int i = 0; i < attributes_count; i ++) {
		delete attributes[i];
	}
	delete[] attributes;

	destructor(this->rva);
	free(this->rva);

	for (int i = 0; i < num_RuntimeVisibleTypeAnnotations; i ++) {
		destructor(&rvta[i]);
	}
	free(rvta);
}

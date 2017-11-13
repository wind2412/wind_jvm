/*
 * field.cpp
 *
 *  Created on: 2017年11月10日
 *      Author: zhengxiaolin
 */

#include "runtime/field.hpp"
#include "classloader.hpp"

Field_info::Field_info(shared_ptr<InstanceKlass> klass, field_info & fi, cp_info **constant_pool) : klass(klass) {	// must be 0, 6, 7, 13, 14, 15, 18, 19
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

				// TODO: Annotations...

			default:{
				std::cerr << "Annotations are TODO! attribute_tag == " << attribute_tag << std::endl;
//				assert(false);
			}
		}
	}

	value_size = parse_field_descriptor(descriptor);
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
				this->true_type = loader->loadClass(descriptor.substr(1, descriptor.size() - 2));		// 这里写错了，写成了 descriptor，竟然出现了诡异的 EXC_BAD_ACCESS (code=2, ...) 错误...... 而且，显示是 wcout 输出那一行错误，但是却怎么也看不出来......要引以为戒啊！！这种错误，很有可能已经在很早很早之前就发生了......
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

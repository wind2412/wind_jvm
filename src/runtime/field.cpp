/*
 * field.cpp
 *
 *  Created on: 2017年11月10日
 *      Author: zhengxiaolin
 */

#include "runtime/field.hpp"

Field_info::Field_info(field_info & fi, cp_info **constant_pool) {	// must be 0, 6, 7, 13, 14, 15, 18, 19
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
				assert(false);
			}
		}
	}

	value_size = parse_field_descriptor(descriptor);
}



#include "runtime/method.hpp"

Method::Method(shared_ptr<InstanceKlass> klass, method_info & mi, cp_info **constant_pool) : klass(klass) {
	assert(constant_pool[mi.name_index-1]->tag == CONSTANT_Utf8);
	name = ((CONSTANT_Utf8_info *)constant_pool[mi.name_index-1])->convert_to_Unicode();
	assert(constant_pool[mi.descriptor_index-1]->tag == CONSTANT_Utf8);
	descriptor = ((CONSTANT_Utf8_info *)constant_pool[mi.descriptor_index-1])->convert_to_Unicode();
	access_flags = mi.access_flags;

	// move!!! important!!!
	this->attributes = mi.attributes;
	mi.attributes = nullptr;

	// set to 0!!! important!!!
	this->attributes_count = mi.attributes_count;
	mi.attributes_count = 0;

	for(int i = 0; i < this->attributes_count; i ++) {
		int attribute_tag = peek_attribute(this->attributes[i]->attribute_name_index, constant_pool);
		switch (attribute_tag) {	// must be 1, 3, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 22
			case 1: {	// Code
				code = (Code_attribute *)this->attributes[i];
				break;
			}
			case 3: {	// Exception
				exceptions = (Exceptions_attribute *)this->attributes[i];
				break;
			}
			case 7: {	// Signature
				signature_index = ((Signature_attribute *)this->attributes[i])->signature_index;
				break;
			}

			// TODO: 支持更多 attributes

			default:{
				std::cerr << "Annotations are TODO! attribute_tag == " << attribute_tag << std::endl;
				assert(false);
			}
		}

	}
}

#ifndef __METHOD_HPP__
#define __METHOD_HPP__

#include <string>
#include <cassert>
#include "class_parser.hpp"

using std::wstring;

class InstanceKlass;

/**
 * single Method. save in InstanceKlass.
 */
class Method {
private:
	// InstanceKlass
	InstanceKlass *klass;
	// method basic
	wstring name;
	wstring descriptor;

	// constant pool ** use for <code> and so on
//	cp_info **constant_pool;

	// code attribute
	Code_attribute *code;

	// exception attribute
	Exceptions_attribute *exceptions;

	// TODO: 支持更多 attributes, Annotations.

public:
	Method(InstanceKlass *klass, const method_info & mi, cp_info **constant_pool) : klass(klass) {
		assert(constant_pool[mi.name_index-1]->tag == CONSTANT_Utf8);
		name = ((CONSTANT_Utf8_info *)constant_pool[mi.name_index-1])->convert_to_Unicode();
		assert(constant_pool[mi.descriptor_index-1]->tag == CONSTANT_Utf8);
		descriptor = ((CONSTANT_Utf8_info *)constant_pool[mi.descriptor_index-1])->convert_to_Unicode();

		for(int i = 0; i < mi.attributes_count; i ++) {
			int attribute_tag = peek_attribute(mi.attributes[i]->attribute_name_index, constant_pool);
			switch (attribute_tag) {	// must be 1, 3, 6, 7, 13, 14, 15, 16, 17, 18
				case 1: {	// Code
					code = (Code_attribute *)mi.attributes[i];
					break;
				}
				case 3: {	// Exception
					exceptions = (Exceptions_attribute *)mi.attributes[i];
					break;
				}

				// TODO: 支持更多 attributes
			}

		}
	}

	const wstring & get_name() { return name; }
	const wstring & get_descriptor() { return descriptor; }
//	bool operator< (const Method & m) {
//		return (name < m.name) ? true : (name > m.name) ? false : descriptor < m.descriptor;		// 先比较 name 在比较 descriptor
//	}
};



#endif

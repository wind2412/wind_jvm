#ifndef __FIELD_HPP__
#define __FIELD_HPP__

#include <string>
#include <class_parser.hpp>
#include <cassert>

using std::wstring;

class Field {
private:
	wstring name;
	wstring descriptor;
	field_info *real;
public:
	Field(field_info *fi, cp_info **constant_pool) : real(fi) {
		assert(constant_pool[fi->name_index-1]->tag == CONSTANT_Utf8);
		name = ((CONSTANT_Utf8_info *)constant_pool[fi->name_index-1])->convert_to_Unicode();
		assert(constant_pool[fi->descriptor_index-1]->tag == CONSTANT_Utf8);
		descriptor = ((CONSTANT_Utf8_info *)constant_pool[fi->descriptor_index-1])->convert_to_Unicode();
	}
	const wstring & get_name() { return name; }
	const wstring & get_index() { return descriptor; }
	bool operator< (const Field & f) {
		return (name < f.name) ? true : (name > f.name) ? false : descriptor < f.descriptor;		// 先比较 name 在比较 descriptor
	}
};



#endif

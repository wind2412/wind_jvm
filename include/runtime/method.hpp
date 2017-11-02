#ifndef __METHOD_HPP__
#define __METHOD_HPP__

#include <string>
#include <class_parser.hpp>
#include <cassert>

using std::wstring;

class Method {
private:
	wstring name;
	wstring descriptor;
	method_info *real;
public:
	Method(method_info *mi, cp_info **constant_pool) : real(mi) {
		assert(constant_pool[mi->name_index-1]->tag == CONSTANT_Utf8);
		name = ((CONSTANT_Utf8_info *)constant_pool[mi->name_index-1])->convert_to_Unicode();
		assert(constant_pool[mi->descriptor_index-1]->tag == CONSTANT_Utf8);
		descriptor = ((CONSTANT_Utf8_info *)constant_pool[mi->descriptor_index-1])->convert_to_Unicode();
	}
	const wstring & get_name() { return name; }
	const wstring & get_index() { return descriptor; }
	bool operator< (const Method & m) {
		return (name < m.name) ? true : (name > m.name) ? false : descriptor < m.descriptor;		// 先比较 name 在比较 descriptor
	}
};



#endif

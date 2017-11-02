#ifndef __INTERFACE_HPP__
#define __INTERFACE_HPP__

#include <string>
#include <class_parser.hpp>
#include <cassert>

using std::wstring;

class Interface {
private:
	wstring name;
	u2 index;		// backup constant pool index
public:
	Interface(u2 index, cp_info **constant_pool) : index(index) {
		assert(constant_pool[index-1]->tag == CONSTANT_Class);
		name = ((CONSTANT_Utf8_info *)constant_pool[((CONSTANT_CS_info *)constant_pool[index-1])->index-1])->convert_to_Unicode();
	}
	const wstring & get_name() { return name; }
};






#endif

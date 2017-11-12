#ifndef __METHOD_HPP__
#define __METHOD_HPP__

#include <string>
#include <cassert>
#include <memory>
#include "class_parser.hpp"

using std::wstring;
using std::shared_ptr;

class InstanceKlass;

/**
 * single Method. save in InstanceKlass.
 */
class Method {
private:
	// InstanceKlass
	shared_ptr<InstanceKlass> klass;
	// method basic
	wstring name;
	wstring descriptor;
	u2 access_flags;

	// constant pool ** use for <code> and so on
//	cp_info **constant_pool;

	// Attributes: 1, 3, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 22
	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	// TODO: code attribute
	Code_attribute *code;
	// TODO: exception attribute
	Exceptions_attribute *exceptions = nullptr;
	u2 signature_index;

	// TODO: 支持更多 attributes, Annotations.

public:
	bool is_static() { return (this->access_flags & ACC_STATIC) == ACC_STATIC; }
	bool is_public() { return (this->access_flags & ACC_PUBLIC) == ACC_PUBLIC; }
	bool is_void() { return descriptor[descriptor.size()-1] == L'V'; }
	bool is_main() { return is_static() && is_public() && is_void(); }
	bool is_native() { return (this->access_flags & ACC_NATIVE) == ACC_NATIVE; }
	wstring return_type() { return descriptor.substr(descriptor.find_first_of(L")")+1); }
public:
	Method(shared_ptr<InstanceKlass> klass, method_info & mi, cp_info **constant_pool);
	const wstring & get_name() { return name; }
	const wstring & get_descriptor() { return descriptor; }
	const Code_attribute *get_code() { return code; }
	void print() { std::wcout << name << ":" << descriptor; }
	shared_ptr<InstanceKlass> get_klass() { return klass; }
	~Method() {
		for (int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes;
	}
};



#endif

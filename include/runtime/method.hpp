#ifndef __METHOD_HPP__
#define __METHOD_HPP__

#include <string>
#include <cassert>
#include <memory>
#include <unordered_map>
#include "runtime/constantpool.hpp"
#include "class_parser.hpp"
#include "annotation.hpp"
#include "utils/synchronize_wcout.hpp"
#include <list>
#include "utils/lock.hpp"

using std::wstring;
using std::unordered_map;
using std::list;

class MirrorOop;
class InstanceKlass;
class Method;

class Method_Pool {
private:
	static Lock & method_pool_lock();
private:
	static list<Method *> & method_pool();
public:
	static void put(Method *method);
	static void cleanup();
};

/**
 * single Method. save in InstanceKlass.
 */
class Method {
private:
	// InstanceKlass
	InstanceKlass *klass = nullptr;		// 同理。由于是 this 指针生成的 shared_ptr...... 希望不会循环引用......
	// method basic
	wstring name;
	wstring descriptor;

	wstring real_descriptor;		// a special patch: for MethodHandle.invoke***(Object...). these methods' descritpor must be [[Ljava/lang/Object;]. the real args are here.

	u2 access_flags;

	// constant pool ** use for <code> and so on
//	cp_info **constant_pool;

	// Attributes: 1, 3, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 22
	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	// RuntimeAnnotation
	Parameter_annotations_t *rva = nullptr;		// [1]

	u1 num_RuntimeVisibleParameterAnnotation = 0;
	Parameter_annotations_t *rvpa = nullptr;		// [n]
	CodeStub _rvpa;

	// RuntimeTypeAnnotation [of Method]
	u2 num_RuntimeVisibleTypeAnnotations = 0;
	TypeAnnotation *rvta = nullptr;				// [n]

	// Code attribute
	Code_attribute *code = nullptr;			// TODO: !!! 小心！！ Code 属性中还有一份异常表！！
	LineNumberTable_attribute *lnt = nullptr;					// 用于调试器。   // 而且用于 printStackTrace。因此留下了。
//	LocalVariableTable_attribute *lvt = nullptr;				// 用于调试器。
//	LocalVariableTypeTable_attribute *lvtt = nullptr;			// 用于调试器。
//	StackMapTable_attribute *smt = nullptr;					// 此属性用于虚拟机的类型检查阶段。
	// RuntimeTypeAnnotation [of Code attribute]
	u2 Code_num_RuntimeVisibleTypeAnnotations = 0;
	TypeAnnotation *Code_rvta = nullptr;			// [n]

	Exceptions_attribute *exceptions = nullptr;
	bool parsed = false;
	unordered_map<wstring, Klass *> exceptions_tb;

	u2 signature_index = 0;
	Element_value *ad = nullptr;					// [1]
	CodeStub _ad;
//	MethodParameters_attribute *mp = nullptr;		// in fact, in my vm, this is of no use.	@Deprecated.

public:
	bool is_static() { return (this->access_flags & ACC_STATIC) == ACC_STATIC; }
	bool is_public() { return (this->access_flags & ACC_PUBLIC) == ACC_PUBLIC; }
	bool is_private() { return (this->access_flags & ACC_PRIVATE) == ACC_PRIVATE; }
	bool is_void() { return descriptor[descriptor.size()-1] == L'V'; }
	bool is_return_primitive() { return descriptor[descriptor.size()-1] != L';'; }
	bool is_main() { return is_static() && is_public() && is_void() && name == L"main" && descriptor == L"([Ljava/lang/String;)V"; }
	bool is_native() { return (this->access_flags & ACC_NATIVE) == ACC_NATIVE; }
	bool is_abstract() { return (this->access_flags & ACC_ABSTRACT) == ACC_ABSTRACT; }
	bool is_synchronized() { return (this->access_flags & ACC_SYNCHRONIZED) == ACC_SYNCHRONIZED; }
	wstring return_type() { return return_type(this->descriptor); }
	u2 get_flag() { return access_flags; }
public:
	static wstring return_type(const wstring & descriptor) { return descriptor.substr(descriptor.find_first_of(L")")+1); }
	static vector<MirrorOop *> parse_argument_list(const wstring & descriptor);
	static MirrorOop *parse_return_type(const wstring & return_type);
public:
	void set_real_descriptor (const wstring & real_descriptor) { this->real_descriptor = real_descriptor; }	// for MethodHandle.invoke**(...) only.
	vector<MirrorOop *> if_didnt_parse_exceptions_then_parse();
	vector<MirrorOop *> parse_argument_list();
	MirrorOop *parse_return_type();
	int get_java_source_lineno(int pc_no);
public:
	bool has_annotation_name_in_method(const wstring & name) {
		if (rvpa != nullptr && rvpa->has_annotation_name(name)) return true;
		if (rva != nullptr && rva->has_annotation_name(name))	return true;
		if (rvta != nullptr && rvta->has_annotation_name(name))	return true;
		return false;
	}
	void print_all_attribute_name() {		// for debug
#ifdef DEBUG
		sync_wcout{} << "===-------------- print attributes name -------------------===" << std::endl;
		sync_wcout{} << "--- Runtime Visible Annotations: \n";
		if (rva != nullptr)
			for (int i = 0; i < rva->num_annotations; i ++) {
				sync_wcout{} << rva->annotations[i].type << std::endl;
			}
		else sync_wcout{} << "none." << std::endl;
		sync_wcout{} << "--- Runtime Visible Parameter Annotations: \n";
		if (rvpa != nullptr)
			for (int i = 0; i < num_RuntimeVisibleParameterAnnotation; i ++) {
				for (int j = 0; j < rvpa[i].num_annotations; j ++) {
					sync_wcout{} << rvpa[i].annotations[j].type << std::endl;
				}
			}
		else sync_wcout{} << "none." << std::endl;
		sync_wcout{} << "--- Runtime Visible Type Annotations: \n";
		if (rvta != nullptr)
			for (int i = 0; i < num_RuntimeVisibleTypeAnnotations; i ++) {
				sync_wcout{} << rvta[i].anno->type << std::endl;
			}
		else sync_wcout{} << "none." << std::endl;
		sync_wcout{} << "===--------------------------------------------------------===" << std::endl;
#endif
	}
public:
	bool operator== (const Method & rhs) const {
		return this->name == rhs.name && this->descriptor == rhs.descriptor;
	}
public:
	Method(InstanceKlass *klass, method_info & mi, cp_info **constant_pool);
	const wstring & get_name() { return name; }
	const wstring & get_descriptor() { return descriptor; }
	const Code_attribute *get_code() { return code; }
	InstanceKlass *get_klass() { return klass; }
	wstring parse_signature();
	void print() { sync_wcout{} << name << ":" << descriptor; }
	CodeStub *get_rva() { if (rva) return &rva->stub; else return nullptr;}
	CodeStub *get_rvpa() { if (rvpa) return &this->_rvpa; else return nullptr;}
	CodeStub *get_ad() { if (ad) return &this->_ad; else return nullptr;}
	int where_is_catch(int cur_pc, InstanceKlass *cur_excp);

	~Method();
};



#endif

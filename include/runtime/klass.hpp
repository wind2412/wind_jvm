/*
 * klass.hpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_KLASS_HPP_
#define INCLUDE_RUNTIME_KLASS_HPP_

#include "method.hpp"
#include "class_parser.hpp"
#include <unordered_map>
#include <vector>
#include <utility>
#include <list>
#include "utils/synchronize_wcout.hpp"
#include "utils/lock.hpp"

using std::unordered_map;
using std::vector;
using std::pair;
using std::list;

//#define KLASS_DEBUG

enum Type {
	BYTE,
	BOOLEAN,
	CHAR,
	SHORT,
	INT,
	FLOAT,
	LONG,
	DOUBLE,
	OBJECT,		// emmm. puzzled			// maybe this is not BasicType.
	ARRAY,		// emmm. more puzzled	// as above.
	VOID,		// emmm. most puzzled...	// ...	// got it. Please see [hotspot/src/cpu/x86/vm/sharedRuntime_x86_32.cpp:460 in OpenJDK8.] We will get the answer that T_VOID is for the `missing half` of Long and Double in constant_pool.
};

enum ClassType {
	InstanceClass,
	ObjArrayClass,
	TypeArrayClass,
};

enum OopType {
	_InstanceOop,
	_BasicTypeOop,
	_ObjArrayOop,
	_TypeArrayOop,
};

class Oop;
class MirrorOop;
class rt_constant_pool;

Type get_type(const wstring & name);		// in fact use wchar_t is okay.

class Rt_Pool {
private:
	static Lock & rt_pool_lock();
private:
	static list<rt_constant_pool *> & rt_pool();
public:
	static void put(rt_constant_pool *pool);
	static void cleanup();
};

class GC;

class Klass /*: public std::enable_shared_from_this<Klass>*/ {		// similar to java.lang.Class	-->		metaClass	// oopDesc is the real class object's Class.
	friend GC;
public:
	enum KlassState{NotInitialized, Initializing, Initialized};		// Initializing is to prevent: some method --(invokestatic clinit first)--> <clinit> --(invokestatic other method but must again called clinit first forcely)--> recursive...
protected:
	KlassState state = KlassState::NotInitialized;
protected:
	ClassType classtype;

	wstring name;		// this class's name		// use constant_pool single string but not copy.	// java/lang/Object
	u2 access_flags;		// this class's access flags

	MirrorOop *java_mirror = nullptr;	// java.lang.Class's object oop!!	// A `MirrorOop` object.

	Klass * parent = nullptr;
public:
	KlassState get_state() { return state; }
	void set_state(KlassState s) { state = s; }
	Klass *get_parent() { return parent; }
	void set_parent(Klass * parent) { this->parent = parent; }
	int get_access_flags() { return access_flags; }
	void set_access_flags(int access_flags) { this->access_flags = access_flags; }
	wstring get_name() { return this->name; }
	void set_name(const wstring & name) { this->name = name; }
	ClassType get_type() { return classtype; }
	MirrorOop *get_mirror() { return java_mirror; }
	void set_mirror(MirrorOop *mirror) { java_mirror = mirror; }
public:
	bool is_interface() { return (this->access_flags & ACC_INTERFACE) == ACC_INTERFACE; }
protected:
	Klass(const Klass &);
	Klass operator= (const Klass &);
public:
	Klass() {}
	virtual ~Klass() {}
};

class Field_info;
class rt_constant_pool;
class InstanceOop;

class InstanceKlass : public Klass {
	friend InstanceOop;
	friend MirrorOop;
	friend GC;
private:
//	ClassFile *cf;		// origin non-dynamic constant pool
	ClassLoader *loader;
	MirrorOop *java_loader = nullptr;		// maybe the Launcher$AppClassLoader.

	u2 constant_pool_count;
	cp_info **constant_pool;

	// interfaces
	unordered_map<wstring, InstanceKlass *> interfaces;
	// should add message to recode whether this field is parent klass's field, for java/lang/Class.getDeclaredFields0()...
	unordered_map<int, bool> is_this_klass_field;		// don't use vector<bool>...		// use fields_layout.second.first as index.
	unordered_map<wstring, pair<int, Field_info *>> fields_layout;			// non-static field layout. [values are in oop]. <classname+':'+name+':'+descriptor, <fields' offset, Field_info>>
	unordered_map<wstring, pair<int, Field_info *>> static_fields_layout;	// static field layout.	<name+':'+descriptor, <static_fields' offset, Field_info>>
	int total_non_static_fields_num = 0;
	int total_static_fields_num = 0;
//	Oop **static_fields = nullptr;												// static field values. [non-static field values are in oop].
	vector<Oop *> static_fields;
	// static methods + vtable + itable
	// TODO: miranda Method !!					// I cancelled itable. I think it will copy from parents' itable and all interface's itable, very annoying... And it's efficiency in my spot based on looking up by wstring, maybe lower than directly looking up...
	unordered_map<wstring, Method *> vtable;		// this vtable save's all father's vtables and override with this class-self. save WITHOUT private/static methods.(including final methods)
	unordered_map<wstring, pair<int, Method *>> methods;	// all methods. These methods here are only for parsing constant_pool. Because `invokestatic`, `invokespecial` directly go to the constant_pool to get the method. WILL NOT go into the Klass to find !! [the `pair<int, ...>` 's int is the slot number. for: sun/reflect/NativeConstructorAccessorImpl-->newInstance0]
	// constant pool
	rt_constant_pool *rt_pool;

	// Attributes
	// 4, 5, 6, 7, 8, 9, 13, 14, 15, 18, 19, 21
	u2 attributes_count;
	attribute_info **attributes;

	InnerClasses_attribute *inner_classes = nullptr;
	EnclosingMethod_attribute *enclosing_method = nullptr;
	wstring sourceFile;
	u2 signature_index = 0;
	BootstrapMethods_attribute *bm = nullptr;

	Parameter_annotations_t *rva = nullptr;

	u2 num_RuntimeVisibleTypeAnnotations = 0;
	TypeAnnotation *rvta = nullptr;

	// for Anonymous Klass only:
	InstanceKlass * host_klass = nullptr;		// if this klass is not Anonymous Klass, will be nullptr.

private:
	void parse_methods(ClassFile *cf);
	void parse_fields(ClassFile *cf);
	void parse_superclass(ClassFile *cf, ClassLoader *loader);
	void parse_interfaces(ClassFile *cf, ClassLoader *loader);
	void parse_attributes(ClassFile *cf);
public:
	void parse_constantpool(ClassFile *cf, ClassLoader *loader);	// only initialize.
public:
	Method *get_static_void_main();
	void initialize_final_static_field();
	wstring parse_signature();
	wstring get_source_file_name();
	auto get_enclosing_method() { return enclosing_method; }
	auto get_inner_class() { return inner_classes; }
public:
	vector<Oop *> & get_static_fields_addr() { return static_fields; }
private:
	void initialize_field(unordered_map<wstring, pair<int, Field_info *>> & fields_layout, vector<Oop *> & fields);		// initializer for parse_fields() and InstanceOop's Initialization
public:
	InstanceKlass *get_hostklass() { return host_klass; }
	void set_hostklass(InstanceKlass *hostklass) { host_klass = hostklass; }
	MirrorOop *get_java_loader() { return this->java_loader; }
	pair<int, Field_info *> get_field(const wstring & descriptor);	// [name + ':' + descriptor]
	Method *get_class_method(const wstring & signature, bool search_interfaces = true);	// [name + ':' + descriptor]		// not only search in `this`, but also in `interfaces` and `parent`!! // You shouldn't use it except pasing rt_pool and ByteCode::invokeInterface !!
	Method *get_this_class_method(const wstring & signature);		// [name + ':' + descriptor]		// we should usually use this method. Because when when we find `<clinit>`, the `get_class_method` can get parent's <clinit> !!! if this has a <clinit>, too, Will go wrong.
	Method *get_interface_method(const wstring & signature);		// [name + ':' + descriptor]
	int non_static_field_num() { return total_non_static_fields_num; }
	bool get_static_field_value(Field_info *field, Oop **result);		// self-maintain a ptr to pass in...
	void set_static_field_value(Field_info *field, Oop *value);
	bool get_static_field_value(const wstring & signature, Oop **result);			// use for forging String Oop at parsing constant_pool. However I don't no static field is of use ?
	void set_static_field_value(const wstring & signature, Oop *value);		// as above.
	Method *search_vtable(const wstring & signature);
	rt_constant_pool *get_rtpool() { return rt_pool; }
	ClassLoader *get_classloader() { return this->loader; }
	bool check_interfaces(const wstring & signature);		// find signature is `this_klass`'s parent interface.
	bool check_interfaces(InstanceKlass *klass);
	bool check_parent(const wstring & signature) {
		if (parent == nullptr)	return false;		// java/lang/Object can't be parent of itself!!
		bool success1 = parent->get_name() == signature;
		bool success2 = (parent == nullptr) ? false : ((InstanceKlass *)parent)->check_parent(signature);
		return success1 || success2;
	}
	bool check_parent(InstanceKlass *klass) {
		if (parent == nullptr)	return false;		// java/lang/Object can't be parent of itself!!
		bool success1 = parent == klass;
		bool success2 = (parent == nullptr) ? false : ((InstanceKlass *)parent)->check_parent(klass);
		return success1 || success2;
	}
	InstanceOop* new_instance();
	Method *search_method_in_slot(int slot);
private:		// for Unsafe
	int get_static_field_offset(const wstring & signature);
public:		// for Unsafe
	int get_all_field_offset(const wstring & BIG_signature);
public:
	bool is_interface() { return (this->access_flags & ACC_INTERFACE) == ACC_INTERFACE; }
public:		// for reflection.
	vector<pair<int, Method *>> get_constructors();
	vector<pair<int, Method *>> get_declared_methods();
public:		// for invokedynamic.
	bool is_in_vtable(Method *m) { wstring signature = m->get_name() + L":" + m->get_descriptor(); return vtable.find(signature) != vtable.end(); }
	const auto & get_field_layout() { return this->fields_layout; }
	const auto & get_static_field_layout() { return this->static_fields_layout; }
	BootstrapMethods_attribute *get_bm() { return this->bm; }
private:
	InstanceKlass(const InstanceKlass &);
public:
	InstanceKlass(ClassFile *cf, ClassLoader *loader, MirrorOop *java_loader = nullptr, ClassType type = ClassType::InstanceClass);	// https://stackoverflow.com/questions/2290733/initialize-parents-protected-members-with-initialization-list-c
	~InstanceKlass();
};

class MirrorOop;
class ArrayOop;

class MirrorKlass : public InstanceKlass {		// this class, only used to static_cast an InstanceKlass to get the method in MirrorKlass. can't be instantiationed.
private:
	MirrorKlass();
public:
	MirrorOop *new_mirror(Klass * mirrored_who, MirrorOop *loader);
};

class ArrayKlass : public Klass {
	friend GC;
private:
	ClassLoader *loader;
	MirrorOop *java_loader = nullptr;

	int dimension;			// (n) dimension (this)
	Klass * higher_dimension = nullptr;	// (n+1) dimension
	Klass * lower_dimension = nullptr;	// (n-1) dimension
	// TODO: vtable
	// TODO: mirror: reflection support

public:
	Klass * get_higher_dimension() { return higher_dimension; }
	void set_higher_dimension(Klass * higher) { higher_dimension = higher; }
	Klass * get_lower_dimension() { return lower_dimension; }
	void set_lower_dimension(Klass * lower) { lower_dimension = lower; }
	int get_dimension() { return dimension; }
	ArrayOop* new_instance(int length);
private:
	ArrayKlass(const ArrayKlass &);
public:
	MirrorOop *get_java_loader() { return this->java_loader; }
	Method *get_class_method(const wstring & signature) {
		Method *target = ((InstanceKlass *)parent)->get_class_method(signature);
		assert(target != nullptr);
		return target;
	}
public:
	ArrayKlass(int dimension, ClassLoader *loader, Klass * lower_dimension, Klass * higher_dimension, MirrorOop *java_loader, ClassType classtype);
	~ArrayKlass() {};
};

class TypeArrayKlass : public ArrayKlass {
private:
	Type type;		// I dont' want to set the MAXLENGTH.
public:
	Type get_basic_type() { return type; }
private:
	TypeArrayKlass(const TypeArrayKlass &);
public:
	TypeArrayKlass(Type type, int dimension, ClassLoader *loader, Klass * lower_dimension, Klass * higher_dimension, MirrorOop *java_loader = nullptr, ClassType classtype = ClassType::TypeArrayClass);
	~TypeArrayKlass() {};
};

class ObjArrayKlass : public ArrayKlass {
private:
	InstanceKlass *element_klass = nullptr;		// e.g. java.lang.String
public:
	InstanceKlass *get_element_klass() { return element_klass; }
private:
	ObjArrayKlass(const ObjArrayKlass &);
public:
	ObjArrayKlass(InstanceKlass *element, int dimension, ClassLoader *loader, Klass * lower_dimension, Klass * higher_dimension, MirrorOop *java_loader = nullptr, ClassType classtype = ClassType::ObjArrayClass);
	~ObjArrayKlass() {};
};

#endif /* INCLUDE_RUNTIME_KLASS_HPP_ */

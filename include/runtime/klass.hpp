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
#include <memory>
#include <utility>

using std::unordered_map;
using std::vector;
using std::shared_ptr;
using std::pair;

/**
 * 这个 Klass 是一定要被 new 出来的！
 * TODO: 方法区。
 */

enum Type {		// TODO: 如果这里发生了变动，那么 TypeArrayKlass 可能需要变动？
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

Type get_type(const wstring & name);		// in fact use wchar_t is okay.

class Klass /*: public std::enable_shared_from_this<Klass>*/ {		// similar to java.lang.Class	-->		metaClass	// oopDesc is the real class object's Class.
public:
	enum State{NotInitialized, Initializing, Initialized};		// Initializing is to prevent: some method --(invokestatic clinit first)--> <clinit> --(invokestatic other method but must again called clinit first forcely)--> recursive...
protected:
	State state = State::NotInitialized;	// TODO: 需不需要？
protected:
	ClassType classtype;

	wstring name;		// this class's name		// use constant_pool single string but not copy.
	u2 access_flags;	// this class's access flags

	shared_ptr<Klass> parent;
	shared_ptr<Klass> next_sibling;
	shared_ptr<Klass> child;
public:
	State get_state() { return state; }
	void set_state(State s) { state = s; }
	shared_ptr<Klass> get_parent() { return parent; }
	void set_parent(shared_ptr<Klass> parent) { this->parent = parent; }
	shared_ptr<Klass> get_next_sibling() { return next_sibling; }
	void set_next_sibling(shared_ptr<Klass> next_sibling) { this->next_sibling = next_sibling; }
	shared_ptr<Klass> get_child() { return child; }
	void set_child(shared_ptr<Klass> child) { this->child = child; }
	int get_access_flags() { return access_flags; }
	void set_access_flags(int access_flags) { this->access_flags = access_flags; }
	wstring get_name() { return name; }
	ClassType get_type() { return classtype; }
public:
	bool is_interface() { return (this->access_flags & ACC_INTERFACE) == ACC_INTERFACE; }
protected:
	Klass(const Klass &);
	Klass operator= (const Klass &);
public:
	Klass() {}
};

class Fields;
class Field_info;
class rt_constant_pool;

/**
 * 对于类中的各种函数：有如下几条规则：
 * 1. 对于 <clinit> 方法，不会有任何 bytecode 能够调用到它；它只能算是 invoke 系列指令的副作用。因为 invoke 指令检查到类没有初始化的时候，会自动调用它。因而不能算是显式调用。
 * 2. 对于 <init> 方法，this 类的 private but not static(this 的 private static 由 invokestatic 来) 方法，以及 super 父类的 non-private 方法 ([x] 会通过 parent ptr 上溯到父类去查找。这说明父类除了 vtable 以外，还是要维护一个排序的 all 方法列表 ----- [√] 好像不太对...其实好像是通过 constant_pool 直接查找。因为这样比较快。)，这三种情况对应 invokespecial 指令。
 * 3. 对于 static 方法，由 invokestatic 指令调用。
 * 4. 对于其他的成员方法，只要不是有接口句柄调用，即便覆盖了接口方法，也是 invokevirtual 指令调用。这时会用到 vtable.
 * 5. 直接在接口句柄上调用，是 invokeinterface 指令调用。这时会用到 itable。
 * 6. 对于 invokedynamic 方法，等用到的时候再说。
 */
class InstanceKlass : public Klass {
	friend Fields;
private:
//	ClassFile *cf;		// origin non-dynamic constant pool
	ClassLoader *loader;

	u2 constant_pool_count;
	cp_info **constant_pool;			// 同样，留一个指针在这里。留作 rt_pool 的 parse 的参照标准用。

	// interfaces
	unordered_map<wstring, shared_ptr<InstanceKlass>> interfaces;
	// fields (non-static / static)
	unordered_map<wstring, pair<int, shared_ptr<Field_info>>> fields_layout;			// non-static field layout. [values are in oop].
	unordered_map<wstring, pair<int, shared_ptr<Field_info>>> static_fields_layout;	// static field layout.	<name+':'+descriptor, <static_fields' offset, Field_info>>
	int total_non_static_fields_bytes = 0;
	int total_static_fields_bytes = 0;
	uint8_t *static_fields = nullptr;												// static field values. [non-static field values are in oop].
	// static methods + vtable + itable
	// TODO: miranda Method !!				// I cancelled itable. I think it will copy from parents' itable and all interface's itable, very annoying... And it's efficiency in my spot based on looking up by wstring, maybe lower than directly looking up...
	vector<shared_ptr<Method>> vtable;		// this vtable save's all father's vtables and override with this class-self. save WITHOUT private/static methods.(including final methods)
	unordered_map<wstring, shared_ptr<Method>> methods;	// all methods. These methods here are only for parsing constant_pool. Because `invokestatic`, `invokespecial` directly go to the constant_pool to get the method. WILL NOT go into the Klass to find !!
	// constant pool
	shared_ptr<rt_constant_pool> rt_pool;

	// TODO: Inner Class!!!

	// Attributes
	// 4, 5, 6, 7, 8, 9, 13, 14, 15, 18, 19, 21
	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	InnerClasses_attribute *inner_classes = nullptr;
	EnclosingMethod_attribute *enclosing_method = nullptr;
	u2 signature_index;
	u2 source_file_index;

	// TODO: Annotations

private:
	void parse_methods(shared_ptr<ClassFile> cf);
	void parse_fields(shared_ptr<ClassFile> cf);
	void parse_superclass(shared_ptr<ClassFile> cf, ClassLoader *loader);
	void parse_interfaces(shared_ptr<ClassFile> cf, ClassLoader *loader);
	void parse_attributes(shared_ptr<ClassFile> cf);
public:
	void parse_constantpool(shared_ptr<ClassFile> cf, ClassLoader *loader);	// only initialize.
public:
	shared_ptr<Method> get_static_void_main();
public:
	pair<int, shared_ptr<Field_info>> get_field(const wstring & signature);				// [name + ':' + descriptor]
	shared_ptr<Method> get_class_method(const wstring & signature);			// [name + ':' + descriptor]
	shared_ptr<Method> get_interface_method(const wstring & signature);		// [name + ':' + descriptor]
	int non_static_field_bytes() { return total_non_static_fields_bytes; }
	unsigned long get_static_field_value(int offset, int size);
	void set_static_field_value(int offset, int size, unsigned long value);
	shared_ptr<rt_constant_pool> get_rtpool() { return rt_pool; }
public:
	bool is_interface() { return (this->access_flags & ACC_INTERFACE) == ACC_INTERFACE; }
private:
	InstanceKlass(const InstanceKlass &);
public:
	InstanceKlass(shared_ptr<ClassFile> cf, ClassLoader *loader, ClassType type = ClassType::InstanceClass);	// 不可以用初始化列表初始化基类的 protected 成员！！ https://stackoverflow.com/questions/2290733/initialize-parents-protected-members-with-initialization-list-c
	~InstanceKlass() {
		for (int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes;
	};
};

class ArrayKlass : public Klass {
private:
	ClassLoader *loader;

	int dimension;			// (n) dimension (this)
	shared_ptr<Klass> higher_dimension;	// (n+1) dimension
	shared_ptr<Klass> lower_dimension;	// (n-1) dimension
	// TODO: vtable
	// TODO: mirror: reflection support

public:
	shared_ptr<Klass> get_higher_dimension() { return higher_dimension; }
	void set_higher_dimension(shared_ptr<Klass> higher) { higher_dimension = higher; }
	shared_ptr<Klass> get_lower_dimension() { return lower_dimension; }
	void set_lower_dimension(shared_ptr<Klass> lower) { lower_dimension = lower; }
	int get_dimension() { return dimension; }
private:
	ArrayKlass(const ArrayKlass &);
public:
	shared_ptr<Method> get_class_method(const wstring & signature) {	// 这里其实就是直接去 java.lang.Object 中去查找。
		shared_ptr<Method> target = std::static_pointer_cast<InstanceKlass>(this->parent)->get_class_method(signature);
		assert(target != nullptr);
		return target;
	}
public:
	ArrayKlass(int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, ClassType classtype);
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
	TypeArrayKlass(Type type, int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, ClassType classtype = ClassType::TypeArrayClass);
	~TypeArrayKlass() {};
};

class ObjArrayKlass : public ArrayKlass {
private:
	shared_ptr<InstanceKlass> element_klass;		// e.g. java.lang.String
public:
	shared_ptr<InstanceKlass> get_element_type() { return element_klass; }
private:
	ObjArrayKlass(const ObjArrayKlass &);
public:
	ObjArrayKlass(shared_ptr<InstanceKlass> element, int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, ClassType classtype = ClassType::ObjArrayClass);
	~ObjArrayKlass() {};
};

#endif /* INCLUDE_RUNTIME_KLASS_HPP_ */

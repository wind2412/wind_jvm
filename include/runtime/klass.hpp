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
#include <bitset>
#include "utils/synchronize_wcout.hpp"

using std::unordered_map;
using std::vector;
using std::shared_ptr;
using std::pair;
using std::bitset;

//#define KLASS_DEBUG

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
	MirrorClass,
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

Type get_type(const wstring & name);		// in fact use wchar_t is okay.

class Klass /*: public std::enable_shared_from_this<Klass>*/ {		// similar to java.lang.Class	-->		metaClass	// oopDesc is the real class object's Class.
public:
	enum KlassState{NotInitialized, Initializing, Initialized};		// Initializing is to prevent: some method --(invokestatic clinit first)--> <clinit> --(invokestatic other method but must again called clinit first forcely)--> recursive...
protected:
	KlassState state = KlassState::NotInitialized;
protected:
	ClassType classtype;

	wstring name;		// this class's name		// use constant_pool single string but not copy.	// java/lang/Object
	u2 access_flags;		// this class's access flags

	MirrorOop *java_mirror = nullptr;	// java.lang.Class's object oop!!	// A `MirrorOop` object.

	shared_ptr<Klass> parent;
	shared_ptr<Klass> next_sibling;
	shared_ptr<Klass> child;
public:
	KlassState get_state() { return state; }
	void set_state(KlassState s) { state = s; }
	shared_ptr<Klass> get_parent() { return parent; }
	void set_parent(shared_ptr<Klass> parent) { this->parent = parent; }
	shared_ptr<Klass> get_next_sibling() { return next_sibling; }
	void set_next_sibling(shared_ptr<Klass> next_sibling) { this->next_sibling = next_sibling; }
	shared_ptr<Klass> get_child() { return child; }
	void set_child(shared_ptr<Klass> child) { this->child = child; }
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
};

class Field_info;
class rt_constant_pool;
class InstanceOop;

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
	friend InstanceOop;
	friend MirrorOop;
private:
//	ClassFile *cf;		// origin non-dynamic constant pool
	ClassLoader *loader;
	MirrorOop *java_loader = nullptr;		// maybe the Launcher$AppClassLoader.

	u2 constant_pool_count;
	cp_info **constant_pool;			// 同样，留一个指针在这里。留作 rt_pool 的 parse 的参照标准用。

	// interfaces
	unordered_map<wstring, shared_ptr<InstanceKlass>> interfaces;
	// fields (non-static / static)					// 这些 layout...... 要加上父类的啊！！！！QAQQAQ 可以直接把父类的 copy 下来......不用递归 OWO.
													// 父类有 static，子类不会去继承......但是会共用。 所以 static_field_layout 不用 copy 下来，只 copy fields_layout 就好......不过，如果查找 set_static_field 的话。【如果本类没有，就必须去 父类去查找...... 这时两个类共用 static。。。】
													// 所以 get_static 和 set_static 也要改...... 要增加 去父类查找 的例程...
													// 而且，也要把 oop 的 get_static 和 set_static 改成一样的逻辑......QAQ
													// TODO: 原来如此......！ java.lang.Class 的生成，仅仅分配空间就可以！因为没有构造函数！！所以......～折磨好几天的问题啊......
	// should add message to recode whether this field is parent klass's field, for java/lang/Class.getDeclaredFields0()...
	unordered_map<int, bool> is_this_klass_field;		// don't use vector<bool>...		// use fields_layout.second.first as index.
	// layouts.		// bug of fields_layout: 由于父类可能有和子类一样名字+描述符的变量，因此会冲突...... 不得不加上了 klass 的命名空间来解决这个问题......悲伤。变成了历史遗留问题......
	unordered_map<wstring, pair<int, shared_ptr<Field_info>>> fields_layout;			// non-static field layout. [values are in oop]. <classname+':'+name+':'+descriptor, <fields' offset, Field_info>>
	unordered_map<wstring, pair<int, shared_ptr<Field_info>>> static_fields_layout;	// static field layout.	<name+':'+descriptor, <static_fields' offset, Field_info>>
	int total_non_static_fields_num = 0;
	int total_static_fields_num = 0;
//	Oop **static_fields = nullptr;												// static field values. [non-static field values are in oop].
	vector<Oop *> static_fields;		// 为了支持 Unsafe 的原子操作，整个 field 必须和 this 有完全相等的距离。因此如果像 Oop ** 一样挂在外边，一个 GC 之后距离肯定变。
									// 实在是太恶心了。居然到这一步才发现 java 的 Unsafe 竟然计算的是绝对距离。但是也没法改了。毕竟我的内存布局和 jvm 的几乎不同。它的
									// Fields 直接挂在此类的后边...
	// static methods + vtable + itable
	// TODO: miranda Method !!				// I cancelled itable. I think it will copy from parents' itable and all interface's itable, very annoying... And it's efficiency in my spot based on looking up by wstring, maybe lower than directly looking up...
	vector<shared_ptr<Method>> vtable;		// this vtable save's all father's vtables and override with this class-self. save WITHOUT private/static methods.(including final methods)
	unordered_map<wstring, pair<int, shared_ptr<Method>>> methods;	// all methods. These methods here are only for parsing constant_pool. Because `invokestatic`, `invokespecial` directly go to the constant_pool to get the method. WILL NOT go into the Klass to find !! [the `pair<int, ...>` 's int is the slot number. for: sun/reflect/NativeConstructorAccessorImpl-->newInstance0]
	// constant pool
	shared_ptr<rt_constant_pool> rt_pool;

	// Attributes
	// 4, 5, 6, 7, 8, 9, 13, 14, 15, 18, 19, 21
	u2 attributes_count;
	attribute_info **attributes;		// 留一个指针在这，就能避免大量的复制了。因为毕竟 attributes 已经产生，没必要在复制一份。只要遍历判断类别，然后分派给相应的 子attributes 指针即可。

	InnerClasses_attribute *inner_classes = nullptr;
	EnclosingMethod_attribute *enclosing_method = nullptr;
	wstring sourceFile;
	u2 signature_index = 0;
	BootstrapMethods_attribute *bm = nullptr;

	Parameter_annotations_t *rva = nullptr;		// TODO: 最终因为 java/lang/reflection/Field 的原因，必须要按照 openjdk 的方式来重新 parse Annotations。即，Array<u1> 的格式......难受得很......唉QAQ

	u2 num_RuntimeVisibleTypeAnnotations = 0;
	TypeAnnotation *rvta = nullptr;

	// for Anonymous Klass only:
	shared_ptr<InstanceKlass> host_klass;		// if this klass is not Anonymous Klass, will be nullptr.

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
	void initialize_final_static_field();
	wstring parse_signature();
	wstring get_source_file_name();
	auto get_enclosing_method() { return enclosing_method; }
	auto get_inner_class() { return inner_classes; }
public:
	vector<Oop *> & get_static_fields_addr() { return static_fields; }
private:
	void initialize_field(unordered_map<wstring, pair<int, shared_ptr<Field_info>>> & fields_layout, vector<Oop *> & fields);		// initializer for parse_fields() and InstanceOop's Initialization
public:
	shared_ptr<InstanceKlass> get_hostklass() { return host_klass; }
	void set_hostklass(shared_ptr<InstanceKlass> hostklass) { host_klass = hostklass; }
	MirrorOop *get_java_loader() { return this->java_loader; }
	pair<int, shared_ptr<Field_info>> get_field(const wstring & BIG_signature);	// [classname + ':' + name + ':' + descriptor]
	shared_ptr<Method> get_class_method(const wstring & signature, bool search_interfaces = true);	// [name + ':' + descriptor]		// not only search in `this`, but also in `interfaces` and `parent`!! // You shouldn't use it except pasing rt_pool and ByteCode::invokeInterface !!
	shared_ptr<Method> get_this_class_method(const wstring & signature);		// [name + ':' + descriptor]		// we should usually use this method. Because when when we find `<clinit>`, the `get_class_method` can get parent's <clinit> !!! if this has a <clinit>, too, Will go wrong.
	shared_ptr<Method> get_interface_method(const wstring & signature);		// [name + ':' + descriptor]
	int non_static_field_num() { return total_non_static_fields_num; }
	bool get_static_field_value(shared_ptr<Field_info> field, Oop **result);		// self-maintain a ptr to pass in...
	void set_static_field_value(shared_ptr<Field_info> field, Oop *value);
	bool get_static_field_value(const wstring & signature, Oop **result);			// use for forging String Oop at parsing constant_pool. However I don't no static field is of use ?
	void set_static_field_value(const wstring & signature, Oop *value);		// as above.
	shared_ptr<Method> search_vtable(const wstring & signature);
	shared_ptr<rt_constant_pool> get_rtpool() { return rt_pool; }
	ClassLoader *get_classloader() { return this->loader; }
	bool check_interfaces(const wstring & signature);		// find signature is `this_klass`'s parent interface.
	bool check_interfaces(shared_ptr<InstanceKlass> klass);
	bool check_parent(const wstring & signature) {
		bool success1 = this->parent->get_name() == signature;
		bool success2 = (this->parent == nullptr) ? false : std::static_pointer_cast<InstanceKlass>(this->parent)->check_parent(signature);
		return success1 || success2;
	}
	bool check_parent(shared_ptr<InstanceKlass> klass) {
		bool success1 = this->parent == klass;
		bool success2 = (this->parent == nullptr) ? false : std::static_pointer_cast<InstanceKlass>(this->parent)->check_parent(klass);
		return success1 || success2;
	}
	InstanceOop* new_instance();
	shared_ptr<Method> search_method_in_slot(int slot);
public:
	bool is_interface() { return (this->access_flags & ACC_INTERFACE) == ACC_INTERFACE; }
public:		// for reflection.
	vector<pair<int, shared_ptr<Method>>> get_constructors();
	vector<pair<int, shared_ptr<Method>>> get_declared_methods();
public:		// for invokedynamic.
	bool is_in_vtable(shared_ptr<Method> m) { return std::find(vtable.begin(), vtable.end(), m) != vtable.end(); }
private:
	InstanceKlass(const InstanceKlass &);
public:
	InstanceKlass(shared_ptr<ClassFile> cf, ClassLoader *loader, MirrorOop *java_loader = nullptr, ClassType type = ClassType::InstanceClass);	// 不可以用初始化列表初始化基类的 protected 成员！！ https://stackoverflow.com/questions/2290733/initialize-parents-protected-members-with-initialization-list-c
	~InstanceKlass();
};

class MirrorOop;
class ArrayOop;

class MirrorKlass : public InstanceKlass {		// this class, only used to static_cast an InstanceKlass to get the method in MirrorKlass. can't be instantiationed.
private:
	MirrorKlass();
public:
	MirrorOop *new_mirror(shared_ptr<Klass> mirrored_who, MirrorOop *loader);
};

class ArrayKlass : public Klass {
private:
	ClassLoader *loader;
	MirrorOop *java_loader = nullptr;

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
	ArrayOop* new_instance(int length);
private:
	ArrayKlass(const ArrayKlass &);
public:
	MirrorOop *get_java_loader() { return this->java_loader; }
	shared_ptr<Method> get_class_method(const wstring & signature) {	// 这里其实就是直接去 java.lang.Object 中去查找。
		shared_ptr<Method> target = std::static_pointer_cast<InstanceKlass>(this->parent)->get_class_method(signature);
		assert(target != nullptr);
		return target;
	}
public:
	ArrayKlass(int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, MirrorOop *java_loader, ClassType classtype);
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
	TypeArrayKlass(Type type, int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, MirrorOop *java_loader = nullptr, ClassType classtype = ClassType::TypeArrayClass);
	~TypeArrayKlass() {};
};

class ObjArrayKlass : public ArrayKlass {
private:
	shared_ptr<InstanceKlass> element_klass;		// e.g. java.lang.String
public:
	shared_ptr<InstanceKlass> get_element_klass() { return element_klass; }
private:
	ObjArrayKlass(const ObjArrayKlass &);
public:
	ObjArrayKlass(shared_ptr<InstanceKlass> element, int dimension, ClassLoader *loader, shared_ptr<Klass> lower_dimension, shared_ptr<Klass> higher_dimension, MirrorOop *java_loader = nullptr, ClassType classtype = ClassType::ObjArrayClass);
	~ObjArrayKlass() {};
};

#endif /* INCLUDE_RUNTIME_KLASS_HPP_ */

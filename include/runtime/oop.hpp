/*
 * oop.hpp
 *
 *  Created on: 2017年11月10日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_OOP_HPP_
#define INCLUDE_RUNTIME_OOP_HPP_

#include "runtime/klass.hpp"
#include "runtime/field.hpp"
#include "utils/monitor.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>

// TODO: 千万不要忘了：所有的 vector 存放 field，都要把 vector 中的 Allocator 置换为 Mempool 的！！！

class Mempool {		// TODO: 此类必须实例化！！内存池 Heap！！适用于多线程！因此 MemAlloc 应该内含一个实例化的 Mempool 对象才行！

};

class MemAlloc {
public:
	static void *allocate(size_t size) {		// TODO: change to real Mem Pool (Heap)
		if (size == 0) {
			return nullptr;		// 这里！！
		}
		void *ptr = malloc(size);
		memset(ptr, 0, size);		// default bzero!
		return ptr;
	}
	static void deallocate(void *ptr) { free(ptr); }
	void *operator new(size_t size) throw() { return allocate(size); }
	void *operator new(size_t size, const std::nothrow_t &) throw() { return allocate(size); }
	void *operator new[](size_t size) throw() { return allocate(size); }
	void *operator new[](size_t size, const std::nothrow_t &) throw() { return allocate(size); }
	void operator delete(void *ptr) { return deallocate(ptr); }
	void operator delete[](void *ptr) { return deallocate(ptr); }
};


class Oop : public MemAlloc {		// 注意：Oop 必须只能使用 new ( = ::operator new + constructor-->::operator new(pointer p) )来分配！！因为要放在堆中。
protected:
	// TODO: HashCode .etc
	OopType ooptype;
	shared_ptr<Klass> klass;
	Monitor m;
public:
	// TODO: HashCode .etc
	shared_ptr<Klass> get_klass() { return klass; }
	OopType get_ooptype() { return ooptype; }
public:
	void enter_monitor() { m.enter(); }
	void wait() { m.wait(); }
	void wait(long macro_sec) { m.wait(macro_sec); }
	void notify() { m.notify(); }
	void notify_all() { m.notify_all(); }
	void leave_monitor() { m.leave(); }
	void force_unlock_when_athrow() { m.force_unlock_when_athrow(); }
public:
	explicit Oop(shared_ptr<Klass> klass, OopType ooptype) : klass(klass), ooptype(ooptype) {}
	Oop(const Oop & rhs) : ooptype(rhs.ooptype), klass(rhs.klass) {}		// Monitor don't copy !!
};

class InstanceOop : public Oop {	// Oop::klass must be an InstanceKlass type.
private:
	int field_length;
	vector<Oop *> fields;	// save a lot of mixed datas. int, float, Long, Reference... if it's Reference, it will point to a Oop object.
public:
	InstanceOop(shared_ptr<InstanceKlass> klass);
	InstanceOop(const InstanceOop & rhs);		// shallow copy
public:		// 以下 8 个方法全部用来赋值。
	bool get_field_value(shared_ptr<Field_info> field, Oop **result);
	void set_field_value(shared_ptr<Field_info> field, Oop *value);
	bool get_field_value(const wstring & BIG_signature, Oop **result);				// use for forging String Oop at parsing constant_pool.
	void set_field_value(const wstring & BIG_signature, Oop *value);					// BIG_signature is: <classname + ':' + name + ':' + descriptor>...
	bool get_static_field_value(shared_ptr<Field_info> field, Oop **result) { return std::static_pointer_cast<InstanceKlass>(klass)->get_static_field_value(field, result); }
	void set_static_field_value(shared_ptr<Field_info> field, Oop *value) { std::static_pointer_cast<InstanceKlass>(klass)->set_static_field_value(field, value); }
	bool get_static_field_value(const wstring & signature, Oop **result) { return std::static_pointer_cast<InstanceKlass>(klass)->get_static_field_value(signature, result); }
	void set_static_field_value(const wstring & signature, Oop *value) { std::static_pointer_cast<InstanceKlass>(klass)->set_static_field_value(signature, value); }
public:
	int get_all_field_offset(const wstring & BIG_signature);				// for Unsafe.
	const vector<Oop *> & get_fields_addr() { return fields; }		// for Unsafe.
private:
	int get_static_field_offset(const wstring & signature);			// for Unsafe
//public:	// deprecated.
//	unsigned long get_value(const wstring & signature);
//	void set_value(const wstring & signature, unsigned long value);
};

class MirrorOop : public InstanceOop {	// for java_mirror. Because java_mirror->klass must be java.lang.Class...... We'd add a varible: mirrored_who.
private:
	shared_ptr<Klass> mirrored_who;		// this Oop being instantiation, must after java.lang.Class loaded !!!
	wstring extra;						// bad design... if it's basic type like int, long, use the `extra`. this time , mirrored_who == nullptr.
public:
	wstring get_extra() { return extra; }
	void set_extra(const wstring & s) { extra = s; }
public:
	MirrorOop(shared_ptr<Klass> mirrored_who);		// 禁止使用任何其他成员变量，比如 OopType!!
public:
	shared_ptr<Klass> get_mirrored_who() { return mirrored_who; }
	void set_mirrored_who(shared_ptr<Klass> mirrored_who) { this->mirrored_who = mirrored_who; }
	const auto & get_mirrored_all_fields() { return std::static_pointer_cast<InstanceKlass>(mirrored_who)->fields_layout; }
	const auto & get_mirrored_all_static_fields() { return std::static_pointer_cast<InstanceKlass>(mirrored_who)->static_fields_layout; }
	bool is_the_field_owned_by_this(int offset) {
		auto & this_fields_map = std::static_pointer_cast<InstanceKlass>(mirrored_who)->is_this_klass_field;
		auto iter = this_fields_map.find(offset);
//assert(iter != this_fields_map.end());
//		std::wcout << "searching..." << iter->
		assert (iter != this_fields_map.end()); // 那么一定在 static field 中。this 的 field 没有！调用者需要在 static field 中找。
		return iter->second;
	}
//	bool is_the_static_field_owned_by_this(const wstring & signature) {
//		auto & this_static_field_map = std::static_pointer_cast<InstanceKlass>(mirrored_who)->static_fields_layout;
//		return this_static_field_map.find(signature) != this_static_field_map.end();
//	}
};

class ArrayOop : public Oop {
protected:
							// length 即 capacity！！是一个东西！vector 这种，由于 capacity 是 malloc 的内存，length 是 placement new 出来的对象，才有分别！这里一上来对象就全了，没有就是 null，所以没有区别！！
	vector<Oop *> buf;		// 注意：这是一个指针数组！！内部全部是指针！这样设计是为了保证 ArrayOop 内部可以嵌套 ArrayOop 的情况，而且也非常符合 Java 自身的特点。
							// 这里， java/util/concurrent/ConcurrentHashMap 源码最后几行中，发现内部计算 Unsafe 的数组元素偏移量，是完全通过 ABASE + i << ASHIFT 的。而 i << ASHIFT 就是平台指针的大小，也就是 scale。所以必须固定大小储存。
public:
	ArrayOop(shared_ptr<ArrayKlass> klass, int length, OopType ooptype) : Oop(klass, ooptype) {
		buf.resize(length);
	}
	ArrayOop(const ArrayOop & rhs);
	int get_length() { return buf.size(); }
	int get_dimension() { return std::static_pointer_cast<ArrayKlass>(klass)->get_dimension(); }
	Oop* & operator[] (int index) {
		assert(index >= 0 && index < buf.size());	// TODO: please replace with ArrayIndexOutofBound...
		return buf[index];
	}
	const Oop* operator[] (int index) const {
		return this->operator[](index);
	}
	int get_buf_offset() {		// use for sun/misc/Unsafe...
//		return ((char *)&buf - (char *)this);
		return 0;				// 因为我把 field 挂在了堆中，因此这里返回 0，在 Unsafe.getObjectVolatile() 中解码更加方便～
	}
	~ArrayOop() {
//		for (int i = 0; i < buf.size(); i ++) {
//			MemAlloc::deallocate(buf[i]);			// TODO: 这里的对于 gc 的写法，要好好考虑！！
//		}
	}
};

class TypeArrayOop : public ArrayOop {
public:		// Most inner type of `buf` is BasicTypeOop.
	TypeArrayOop(shared_ptr<TypeArrayKlass> klass, int length) : ArrayOop(klass, length, OopType::_TypeArrayOop) {}
public:

};

class ObjArrayOop : public ArrayOop {
public:		// Most inner type of `buf` is InstanceOop.		// 注意：维度要由自己负责！！并不会进行检查。
	ObjArrayOop(shared_ptr<ObjArrayKlass> klass, int length) : ArrayOop(klass, length, OopType::_ObjArrayOop) {}
public:
};


// Basic Type...

class BasicTypeOop : public Oop {
private:
	Type type;	// only allow for CHAR, INT, FLOAT, LONG, DOUBLE.		[BYTE, BOOLEAN and SHORT are all INT!!!]
public:
	BasicTypeOop(Type type) : Oop(nullptr, OopType::_BasicTypeOop), type(type) {}
	Type get_type() { return type; }
};

//struct ByteOop : public BasicTypeOop {
//	int8_t value;		// data		// 全都有符号......！！
//	ByteOop(int8_t value) : BasicTypeOop(Type::BYTE), value(value) {}
//};
//
//struct BooleanOop : public BasicTypeOop {
//	bool value;		// data
//	BooleanOop(bool value) : BasicTypeOop(Type::BOOLEAN), value(value) {}
//};
//
//struct ShortOop : public BasicTypeOop {
//	short value;		// data
//	ShortOop(short value) : BasicTypeOop(Type::SHORT), value(value) {}
//};
//
//struct CharOop : public BasicTypeOop {
//	unsigned short value;		// data		// [x] must be 16 bits!! for unicode (jchar is unsigned short)	// I modified it to 32 bits... Though of no use, jchar of 16 bits is very bad design......	// finally modified back to short...
//	CharOop(unsigned short value) : BasicTypeOop(Type::CHAR), value(value) {}
//};

struct IntOop : public BasicTypeOop {
	int value;		// data
	IntOop(int value) : BasicTypeOop(Type::INT), value(value) {}
};

struct FloatOop : public BasicTypeOop {
	float value;		// data
	FloatOop(float value) : BasicTypeOop(Type::FLOAT), value(value) {}
};

struct LongOop : public BasicTypeOop {
	long value;		// data
	LongOop(long value) : BasicTypeOop(Type::LONG), value(value) {}
};

struct DoubleOop : public BasicTypeOop {
	double value;		// data
	DoubleOop(float value) : BasicTypeOop(Type::DOUBLE), value(value) {}
};

#endif /* INCLUDE_RUNTIME_OOP_HPP_ */

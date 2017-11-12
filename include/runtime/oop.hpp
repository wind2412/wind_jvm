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

#include <cstdlib>
#include <cstring>
#include <memory>

template <typename Tp, typename Arg1>
void __constructor(Tp *ptr, const Arg1 & arg1)		// 适配一个参数
{
	::new ((void *)ptr) Tp(arg1);
}

template <typename Tp, typename Arg1, typename Arg2>
void __constructor(Tp *ptr, const Arg1 & arg1, const Arg2 & arg2)		// 适配两个参数
{
	::new ((void *)ptr) Tp(arg1, arg2);
}

template <typename Tp, typename Arg1, typename Arg2, typename Arg3>
void __constructor(Tp *ptr, const Arg1 & arg1, const Arg2 & arg2, const Arg3 & arg3)		// 适配三个参数
{
	::new ((void *)ptr) Tp(arg1, arg2, arg3);
}

template <typename Tp, typename ...Args>
void constructor(Tp *ptr, Args &&...args)		// placement new.
{
	__constructor(ptr, std::forward<Args>(args)...);		// 完美转发变长参数
}

template <typename Tp>
void destructor(Tp *ptr)
{
	ptr->~Tp();
}

class Mempool {		// TODO: 此类必须实例化！！内存池 Heap！！适用于多线程！因此 MemAlloc 应该内含一个实例化的 Mempool 对象才行！

};

class MemAlloc {
public:
	static void *allocate(size_t size) {		// TODO: change to real Mem Pool (Heap)
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
	shared_ptr<Klass> klass;
public:
	// TODO: HashCode .etc
	shared_ptr<Klass> get_klass() { return klass; }
public:
	explicit Oop(shared_ptr<Klass> klass) : klass(klass) {}
};

class InstanceOop : public Oop {	// Oop::klass must be an InstanceKlass type.
private:
	int field_length;
	uint8_t *fields = nullptr;	// save a lot of mixed datas. int, float, Long, Reference... if it's Reference, it will point to a Oop object.
public:
	InstanceOop(shared_ptr<InstanceKlass> klass);
private:
	unsigned long get_field_value(int offset, int size);
	void set_field_value(int offset, int size, unsigned long value);
public:
	unsigned long get_value(const wstring & signature);
	void set_value(const wstring & signature, unsigned long value);
};

class BasicTypeOop : public Oop {
private:
	Type type;	// only allow for BYTE, BOOLEAN, CHAR, SHORT, INT, FLOAT, LONG, DOUBLE.
	uint64_t value;
public:
	BasicTypeOop(Type type, uint64_t value) : Oop(nullptr), type(type), value(value) {}
	Type get_type() { return type; }
	uint64_t get_value() { return value; }
};

class ArrayOop : public Oop {
protected:
	int length;
	Oop **buf = nullptr;		// 注意：这是一个指针数组！！内部全部是指针！这样设计是为了保证 ArrayOop 内部可以嵌套 ArrayOop 的情况，而且也非常符合 Java 自身的特点。
public:
	ArrayOop(shared_ptr<ArrayKlass> klass, int length) : Oop(klass), length(length), buf((Oop **)MemAlloc::allocate(sizeof(Oop *) * length)) {}	// **only malloc (sizeof(ptr) * length) !!!!**
	int get_length() { return length; }
	int get_dimension() { return std::static_pointer_cast<ArrayKlass>(klass)->get_dimension(); }
	Oop * operator[] (int index) {
		assert(index >= 0 && index < length);	// TODO: please replace with ArrayIndexOutofBound...
		return buf[index];
	}
	const Oop * operator[] (int index) const {
		return this->operator[](index);
	}
};

class ObjArrayOop : public ArrayOop {
public:		// Most inner type of `buf` is InstanceOop.
	ObjArrayOop(shared_ptr<ObjArrayKlass> klass, int length) : ArrayOop(klass, length) {}
public:
};

class TypeArrayOop : public ArrayOop {
public:		// Most inner type of `buf` is BasicTypeOop.
	TypeArrayOop(shared_ptr<TypeArrayKlass> klass, int length) : ArrayOop(klass, length) {}
public:
};



#endif /* INCLUDE_RUNTIME_OOP_HPP_ */

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

#include <memory>

class MemPool {
public:
	// TODO: use MemPool!!! 而且这 new new[] delete delete[] 的 throw() 声明要搞明白啊....QAQ
	void *operator new (size_t size) throw() { return malloc(size); }
	void *operator new (size_t size, const std::nothrow_t & nothrow_constant) throw() { return malloc(size); }
	void *operator new[] (size_t size) throw() { return malloc(size); }
	void *operator new[] (size_t size, const std::nothrow_t & nothrow_constant) throw() { return malloc(size); }
	void operator delete (void *p) throw() { free(p); }
	void operator delete[] (void *p) throw() { free(p); }
};


class Oop : public MemPool {		// 注意：Oop 必须只能使用 new ( = ::operator new + constructor-->::operator new(pointer p) )来分配！！因为要放在堆中。
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

class TypeArrayOop : public Oop {
private:
	int length;
public:
	TypeArrayOop(shared_ptr<TypeArrayKlass> klass);
public:
};



#endif /* INCLUDE_RUNTIME_OOP_HPP_ */

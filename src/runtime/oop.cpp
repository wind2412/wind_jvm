/*
 * oop.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/oop.hpp"
#include "classloader.hpp"
#include "utils/synchronize_wcout.hpp"

/*===---------------- Memory -------------------===*/
Oop *Mempool::copy(Oop & oop) {
	return oop.copy();		// do not record in `Mempool::oop_handler_pool()`.
}

void *MemAlloc::allocate(size_t size, bool dont_record)
{
	if (size == 0) {
		return nullptr;
	}

	void *ptr = malloc(size);
	memset(ptr, 0, size);		// default bzero!

	// add it to the Mempool
	if (!dont_record) {
		LockGuard lg(mem_lock());
		Mempool::oop_handler_pool().push_back((Oop *)ptr);
	}

	return ptr;
}

void MemAlloc::deallocate(void *ptr)
{
	free(ptr);
}

void *MemAlloc::operator new(size_t size, bool dont_record) throw()
{
	return allocate(size, dont_record);
}

void *MemAlloc::operator new[](size_t size, bool dont_record) throw()
{
	return allocate(size, dont_record);
}

void MemAlloc::operator delete(void *ptr)
{
	return deallocate(ptr);
}

void MemAlloc::operator delete[](void *ptr)
{
	return deallocate(ptr);
}

void MemAlloc::cleanup() {
	LockGuard lg(mem_lock());
	for (auto iter : Mempool::oop_handler_pool()) {
		delete iter;
	}
}

/*===----------------  Oop  -----------------===*/
Oop *Oop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((Oop *)buf, *this);
	return (Oop *)buf;
}

/*===----------------  InstanceOop  -----------------===*/
InstanceOop::InstanceOop(InstanceKlass *klass) : Oop(klass, OopType::_InstanceOop) {
	// alloc non-static-field memory.
	this->field_length = klass->non_static_field_num();
	if (this->field_length != 0) {
		fields.resize(this->field_length, nullptr);
	}

	// initialize BasicTypeOop...
	klass->initialize_field(klass->fields_layout, this->fields);
}

InstanceOop::InstanceOop(const InstanceOop & rhs) : Oop(rhs), field_length(rhs.field_length), fields(rhs.fields)	// TODO: not gc control......
{
}

bool InstanceOop::get_field_value(Field_info *field, Oop **result)
{
	wstring BIG_signature = field->get_klass()->get_name() + L":" + field->get_name() + L":" + field->get_descriptor();
	return this->get_field_value(BIG_signature, result);
}

void InstanceOop::set_field_value(Field_info *field, Oop *value)
{
	wstring BIG_signature = field->get_klass()->get_name() + L":" + field->get_name() + L":" + field->get_descriptor();
	this->set_field_value(BIG_signature, value);
}

bool InstanceOop::get_field_value(const wstring & BIG_signature, Oop **result) 				// use for forging String Oop at parsing constant_pool.
{
	InstanceKlass *instance_klass = ((InstanceKlass *)this->klass);
	auto iter = instance_klass->fields_layout.find(BIG_signature);
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << BIG_signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;

	*result = this->fields[offset];
	return true;
}

void InstanceOop::set_field_value(const wstring & BIG_signature, Oop *value)
{
	InstanceKlass *instance_klass = ((InstanceKlass *)this->klass);
	auto iter = instance_klass->fields_layout.find(BIG_signature);
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << BIG_signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;

	this->fields[offset] = value;
}

int InstanceOop::get_all_field_offset(const wstring & BIG_signature)
{
	// first, search in non-static field.
	InstanceKlass *instance_klass = ((InstanceKlass *)this->klass);
	return instance_klass->get_all_field_offset(BIG_signature);
}

int InstanceOop::get_static_field_offset(const wstring & signature)
{
	InstanceKlass *instance_klass = ((InstanceKlass *)this->klass);
	return instance_klass->get_static_field_offset(signature);
}

Oop *InstanceOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((InstanceOop *)buf, *this);
	return (Oop *)buf;
}

/*===----------------  MirrorOop  -------------------===*/
MirrorOop::MirrorOop(Klass *mirrored_who)
					: InstanceOop(((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class"))),
					  mirrored_who(mirrored_who){}

MirrorOop::MirrorOop(const MirrorOop & rhs)
					: InstanceOop(rhs), mirrored_who(rhs.mirrored_who), extra(rhs.extra) {}

Oop *MirrorOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((MirrorOop *)buf, *this);
	return (Oop *)buf;
}

/*===----------------  TypeArrayOop  -------------------===*/
ArrayOop::ArrayOop(const ArrayOop & rhs) : Oop(rhs), buf(rhs.buf)
{
}

Oop *ArrayOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((ArrayOop *)buf, *this);
	return (Oop *)buf;
}

/*===---------------- BasicTypeOop ---------------------===*/
Oop *BasicTypeOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((BasicTypeOop *)buf, *this);
	return (Oop *)buf;
}

Oop *IntOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((IntOop *)buf, *this);
	return (Oop *)buf;
}

Oop *FloatOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((FloatOop *)buf, *this);
	return (Oop *)buf;
}

Oop *DoubleOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((DoubleOop *)buf, *this);
	return (Oop *)buf;
}

Oop *LongOop::copy()
{
	void *buf = MemAlloc::operator new(sizeof(*this), true);
	constructor((LongOop *)buf, *this);
	return (Oop *)buf;
}


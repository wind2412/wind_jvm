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
void *MemAlloc::allocate(size_t size)
{
	if (size == 0) {
		return nullptr;			// 直接返回！
	}

	LockGuard lg(mem_lock());
	void *ptr = malloc(size);
	memset(ptr, 0, size);		// default bzero!

	// add it to the Mempool
	Mempool::oop_handler_pool().push_back((Oop *)ptr);

	return ptr;
}

void MemAlloc::deallocate(void *ptr)
{
	LockGuard lg(mem_lock());

	// TODO: 这里要怎么规划......是先把所有的全都扫描一遍并且复制到另一堆中，然后把这个 unordered_set 全部删除把...... 这样的话，这里什么也不用写就好了。

//	assert(false);		// 先 assert(false) 了。

	free(ptr);
}

void *MemAlloc::operator new(size_t size) throw()
{
	return allocate(size);
}

void *MemAlloc::operator new[](size_t size) throw()
{
	return allocate(size);
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

/*===----------------  InstanceOop  -----------------===*/
InstanceOop::InstanceOop(InstanceKlass *klass) : Oop(klass, OopType::_InstanceOop) {
	// alloc non-static-field memory.
	this->field_length = klass->non_static_field_num();
//	std::wcout << klass->get_name() << "'s field_size allocate " << this->field_length << " bytes..." << std::endl;	// delete
	if (this->field_length != 0) {
//		fields = new Oop*[this->field_length];			// TODO: not gc control......
//		memset(fields, 0, this->field_length * sizeof(Oop *));		// 啊啊啊啊全部清空别忘了！！

		fields.resize(this->field_length, nullptr);

	}

	// initialize BasicTypeOop...
	klass->initialize_field(klass->fields_layout, this->fields);
}

InstanceOop::InstanceOop(const InstanceOop & rhs) : Oop(rhs), field_length(rhs.field_length), fields(rhs.fields)	// TODO: not gc control......
{
//	memcpy(this->fields, rhs.fields, sizeof(Oop *) * this->field_length);		// shallow copy
}

bool InstanceOop::get_field_value(Field_info *field, Oop **result)		// 这里最终改成了专门给 getField，setField 字节码使用的函数。由于 namespace 不同，因此会用多级父类的名字在 field_layout 中进行查找。
{
	wstring BIG_signature = field->get_klass()->get_name() + L":" + field->get_name() + L":" + field->get_descriptor();
	return this->get_field_value(BIG_signature, result);
}

void InstanceOop::set_field_value(Field_info *field, Oop *value)		// 这里最终改成了专门给 getField，setField 字节码使用的函数。
{
	wstring BIG_signature = field->get_klass()->get_name() + L":" + field->get_name() + L":" + field->get_descriptor();		// bug report: 本来就应该这么调才对...原先写得是什么玩意啊.....搞什么递归... 直接通过 BIG_signature 查不就得了....
	this->set_field_value(BIG_signature, value);
}

bool InstanceOop::get_field_value(const wstring & BIG_signature, Oop **result) 				// use for forging String Oop at parsing constant_pool.
{		// [bug发现] 在 ByteCodeEngine 中，getField 里边，由于我的设计，因此即便是读取 int 也会返回一个 IntOop 的 对象。因此这肯定是错误的... 改为在这里直接取引用，直接解除类型并且取出真值。
	InstanceKlass *instance_klass = ((InstanceKlass *)this->klass);
	auto iter = instance_klass->fields_layout.find(BIG_signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
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

/*===----------------  MirrorOop  -------------------===*/
MirrorOop::MirrorOop(Klass *mirrored_who)			// 注意：在使用 lldb 调试的时候：输入 mirrored_who 其实是在输出 this->mirrored_who...... 而不是这个形参的...... 形参的 mirrored_who 的输出要使用 fr v mirrored_who 来进行打印！......
					: InstanceOop(((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class"))),
					  mirrored_who(mirrored_who){}

/*===----------------  TypeArrayOop  -------------------===*/
ArrayOop::ArrayOop(const ArrayOop & rhs) : Oop(rhs), buf(rhs.buf)	// TODO: not gc control......
{
//	memcpy(this->buf, rhs.buf, sizeof(Oop *) * this->length);		// shallow copy
}


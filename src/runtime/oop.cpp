/*
 * oop.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/oop.hpp"
#include "classloader.hpp"
#include "utils/synchronize_wcout.hpp"

/*===----------------  InstanceOop  -----------------===*/
InstanceOop::InstanceOop(shared_ptr<InstanceKlass> klass) : Oop(klass, OopType::_InstanceOop) {
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

bool InstanceOop::get_field_value(shared_ptr<Field_info> field, Oop **result)		// 这里最终改成了专门给 getField，setField 字节码使用的函数。由于 namespace 不同，因此会用多级父类的名字在 field_layout 中进行查找。
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring descriptor = field->get_name() + L":" + field->get_descriptor();
	// for `this klass` and its parents: (except interfaces. because interfaces' values are `public static final`. It will be get by `iconst_` or `bipush`... and so on.
	while (instance_klass != nullptr) {
		wstring BIG_signature = instance_klass->get_name() + L":" + descriptor;
		auto iter = instance_klass->fields_layout.find(BIG_signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
		if (iter == instance_klass->fields_layout.end()) {
			instance_klass = std::static_pointer_cast<InstanceKlass>(instance_klass->get_parent());
			continue;
		}
		int offset = iter->second.first;
		iter->second.second->if_didnt_parse_then_parse();		// important !!
		*result = this->fields[offset];
		return true;
	}

	std::wcerr << "didn't find field [" << descriptor << "] in InstanceKlass " << this->klass->get_name() << " and its parents!" << std::endl;
	assert(false);

//	return get_field_value(signature, result);
}

void InstanceOop::set_field_value(shared_ptr<Field_info> field, Oop *value)		// 这里最终改成了专门给 getField，setField 字节码使用的函数。
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring descriptor = field->get_name() + L":" + field->get_descriptor();

	// for `this klass` and its parents: (except interfaces. because interfaces' values are `public static final`. It will be get by `iconst_` or `bipush`... and so on.
	while (instance_klass != nullptr) {
		wstring BIG_signature = instance_klass->get_name() + L":" + descriptor;
		auto iter = instance_klass->fields_layout.find(BIG_signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
		if (iter == instance_klass->fields_layout.end()) {
			instance_klass = std::static_pointer_cast<InstanceKlass>(instance_klass->get_parent());
			continue;
		}
		int offset = iter->second.first;
		iter->second.second->if_didnt_parse_then_parse();		// important!!
		this->fields[offset] = value;
		return;
	}

	std::wcerr << "didn't find field [" << descriptor << "] in InstanceKlass " << this->klass->get_name() << " and its parents!" << std::endl;
	assert(false);

//	set_field_value(signature, value);
}

bool InstanceOop::get_field_value(const wstring & BIG_signature, Oop **result) 				// use for forging String Oop at parsing constant_pool.
{		// [bug发现] 在 ByteCodeEngine 中，getField 里边，由于我的设计，因此即便是读取 int 也会返回一个 IntOop 的 对象。因此这肯定是错误的... 改为在这里直接取引用，直接解除类型并且取出真值。
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(BIG_signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << BIG_signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important !!

	*result = this->fields[offset];
	return true;
}

void InstanceOop::set_field_value(const wstring & BIG_signature, Oop *value)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(BIG_signature);
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << BIG_signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important!!

	this->fields[offset] = value;
}

int InstanceOop::get_all_field_offset(const wstring & BIG_signature)
{
	// first, search in non-static field.
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	return instance_klass->get_all_field_offset(BIG_signature);
}

int InstanceOop::get_static_field_offset(const wstring & signature)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	return instance_klass->get_static_field_offset(signature);
}

/*===----------------  MirrorOop  -------------------===*/
MirrorOop::MirrorOop(shared_ptr<Klass> mirrored_who)			// 注意：在使用 lldb 调试的时候：输入 mirrored_who 其实是在输出 this->mirrored_who...... 而不是这个形参的...... 形参的 mirrored_who 的输出要使用 fr v mirrored_who 来进行打印！......
					: InstanceOop(std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class"))),
					  mirrored_who(mirrored_who){}

/*===----------------  TypeArrayOop  -------------------===*/
ArrayOop::ArrayOop(const ArrayOop & rhs) : Oop(rhs), buf(rhs.buf)	// TODO: not gc control......
{
//	memcpy(this->buf, rhs.buf, sizeof(Oop *) * this->length);		// shallow copy
}


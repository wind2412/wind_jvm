/*
 * oop.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/oop.hpp"
#include "classloader.hpp"

/*===----------------  InstanceOop  -----------------===*/
InstanceOop::InstanceOop(shared_ptr<InstanceKlass> klass) : Oop(klass, OopType::_InstanceOop) {
	// alloc non-static-field memory.
	this->field_length = klass->non_static_field_num();
	std::wcout << klass->get_name() << "'s field_size allocate " << this->field_length << " bytes..." << std::endl;	// delete
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

bool InstanceOop::get_field_value(shared_ptr<Field_info> field, Oop **result)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring signature = field->get_name() + L":" + field->get_descriptor();
	return get_field_value(signature, result);
}

void InstanceOop::set_field_value(shared_ptr<Field_info> field, Oop *value)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring signature = field->get_name() + L":" + field->get_descriptor();
	set_field_value(signature, value);
}

bool InstanceOop::get_field_value(const wstring & signature, Oop **result) 				// use for forging String Oop at parsing constant_pool.
{		// [bug发现] 在 ByteCodeEngine 中，getField 里边，由于我的设计，因此即便是读取 int 也会返回一个 IntOop 的 对象。因此这肯定是错误的... 改为在这里直接取引用，直接解除类型并且取出真值。
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important !!

//	// volatile [0]
//	bool is_volatile = iter->second.second->is_volatile();
//	// volatile [1]
//	if (is_volatile) {
//		this->fields[offset]->enter_monitor();
//	}
	// field value not 0, maybe basic type.
	*result = this->fields[offset];
//	// volatile [2]
//	if (is_volatile) {
//		this->fields[offset]->leave_monitor();
//	}
	return true;
}

void InstanceOop::set_field_value(const wstring & signature, Oop *value)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(signature);
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important!!

//	// volatile [0]		// TODO:
//	bool is_volatile = iter->second.second->is_volatile();
//	// volatile [1]
//	if (is_volatile) {
//		this->fields[offset]->enter_monitor();
//	}
	// field value not 0, maybe basic type.
	this->fields[offset] = value;
//	// volatile [2]
//	if (is_volatile) {
//		this->fields[offset]->leave_monitor();
//	}

}

int InstanceOop::get_all_field_offset(const wstring & signature)
{
	// first, search in non-static field.
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
	if (iter == instance_klass->fields_layout.end()) {
		// then, search in static field.
		return get_static_field_offset(signature);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important !!

//	// volatile [0]
//	bool is_volatile = iter->second.second->is_volatile();
//	// volatile [1]
//	if (is_volatile) {
//		this->fields[offset]->enter_monitor();
//	}
	// field value not 0, maybe basic type.
//	// volatile [2]
//	if (is_volatile) {
//		this->fields[offset]->leave_monitor();
//	}

	// 不行，完全支持不了。java 默认所有 同一 klass 的 obj 都是相同的内存布局。我把 fields 挂在外边计算和 this 的距离，根本算不出来！每次都不同！！
	// [√] 只有一种手段可以解决 ———— 变绝对距离成相对距离，就像分布式系统变绝对时间为相对逻辑时间一样！！这样应该可以完美解决！！
	// 方法即是：
	// 这里存放的不是绝对距离，我会把语义完全改变，成为 “和此 oop 存放的 field 的起始地址的相对距离”，而不是 “和此 oop 的 this 指针的绝对距离”！！
	// 这样，GC 也可以用多种算法了！！看来也可以支持复制算法了！开森～
#ifdef DEBUG
	std::wcout << "this: [" << this << "], klass_name:[" << instance_klass->get_name() << "], " << signature << ":[" << &this->fields[offset] << "(offset: " << offset <<")]" << std::endl;
#endif
//	return (char *)&this->fields[offset] - (char *)this;
	return offset;	// vector 是连续内存。
}

int InstanceOop::get_static_field_offset(const wstring & signature)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->static_fields_layout.find(signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
	if (iter == instance_klass->static_fields_layout.end()) {
		std::wcerr << "didn't find static field [" << signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	iter->second.second->if_didnt_parse_then_parse();		// important !!

	// TODO: volatile?

#ifdef DEBUG
	std::wcout << "this: [" << this << "], klass_name:[" << instance_klass->get_name() << "], (static)" << signature << ":[" << &this->fields[offset] << "(encoding: " << offset + instance_klass->non_static_field_num() << ")]" << std::endl;
#endif
	return offset + instance_klass->non_static_field_num();		// 这里需要注意。由于 static 和 non-static 我是分别存放的，而 unsafe 中指定的 offset 是唯一的。这就造成我不知道去 static 里边找还是 non-static 里边找。“两个都找，找到就ok” 的策略一定会引入软件漏洞。因此，采用编码，让 non-static 和 static 的编号永远不会重合。根据 non-static-field-size 来判断去哪里找。
}

/*===----------------  MirrorOop  -------------------===*/
MirrorOop::MirrorOop(shared_ptr<Klass> mirrored_who)			// 注意：在使用 lldb 调试的时候：输入 mirrored_who 其实是在输出 this->mirrored_who...... 而不是这个形参的...... 形参的 mirrored_who 的输出要使用 fr v mirrored_who 来进行打印！......
					: InstanceOop(std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class"))),
					  mirrored_who(mirrored_who){}

/*===----------------  TypeArrayOop  -------------------===*/
ArrayOop::ArrayOop(const ArrayOop & rhs) : Oop(rhs), length(rhs.length), buf(new Oop*[this->length])	// TODO: not gc control......
{
	memcpy(this->buf, rhs.buf, sizeof(Oop *) * this->length);		// shallow copy
}


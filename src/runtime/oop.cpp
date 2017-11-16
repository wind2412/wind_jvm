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
	this->field_length = klass->non_static_field_bytes();
	std::wcout << klass->get_name() << "'s field_size allocate " << this->field_length << " bytes..." << std::endl;	// delete
	if (this->field_length != 0)
		fields = new uint8_t[this->field_length];			// TODO: not gc control......
}

bool InstanceOop::get_field_value(shared_ptr<Field_info> field, uint64_t *result)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring signature = field->get_name() + L":" + field->get_descriptor();
	return get_field_value(signature, result);
}

void InstanceOop::set_field_value(shared_ptr<Field_info> field, uint64_t value)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	wstring signature = field->get_name() + L":" + field->get_descriptor();
	set_field_value(signature, value);
}

bool InstanceOop::get_field_value(const wstring & signature, uint64_t *result) 				// use for forging String Oop at parsing constant_pool.
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(signature);		// non-static field 由于复制了父类中的所有 field (继承)，所以只在 this_klass 中查找！
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	int size = iter->second.second->get_value_size();
	switch (size) {
		case 1:
			*result = *(uint8_t *)(this->fields + offset);
			return true;
		case 2:
			*result = *(uint16_t *)(this->fields + offset);
			return true;
		case 4:
			*result = *(uint32_t *)(this->fields + offset);
			return true;
		case 8:
			*result = *(uint64_t *)(this->fields + offset);
			return true;
		default:{
			std::cerr << "can't get here! size == " << size << std::endl;
			assert(false);
		}
	}
}

void InstanceOop::set_field_value(const wstring & signature, uint64_t value)
{
	shared_ptr<InstanceKlass> instance_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto iter = instance_klass->fields_layout.find(signature);
	if (iter == instance_klass->fields_layout.end()) {
		std::wcerr << "didn't find field [" << signature << "] in InstanceKlass " << instance_klass->name << std::endl;
		assert(false);
	}
	int offset = iter->second.first;
	int size = iter->second.second->get_value_size();
	switch (size) {
		case 1:
			*(uint8_t *)(this->fields + offset) = value;
			break;
		case 2:
			*(uint16_t *)(this->fields + offset) = value;
			break;
		case 4:
			*(uint32_t *)(this->fields + offset) = value;
			break;
		case 8:
			*(uint64_t *)(this->fields + offset) = value;
			break;
		default:{
			std::cerr << "can't get here! size == " << size << std::endl;
			assert(false);
		}
	}
}

//unsigned long InstanceOop::get_value(const wstring & signature) {	// 注意......一个非常重要的问题就是，这里的类型系统是由字节码来提示的：也就是，根据字节码 istore, astore，你可以自动把它们转换为 int/Reference(Oop) 类型......也就是， 这里仅仅返回类型擦除后的 unsigned long(max 8 bytes, 64bits) 就可以。
//	// static / non-static 的信息被我给抹去了。外边不知道调用的是 static 还是 non-static.
//	auto this_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
//	auto target = this_klass->get_field(signature);
//	assert(target.first != -1);	// valid field
//	if (target.second->is_static()) {
//		return this_klass->get_static_field_value(target.second);
//	} else {
//		return this->get_field_value(target.second);
//	}
//}
//
//void InstanceOop::set_value(const wstring & signature, unsigned long value) {
//	auto this_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
//	auto target = this_klass->get_field(signature);
//	assert(target.first != -1);	// valid field
//	if (target.second->is_static()) {
//		this_klass->set_static_field_value(target.second, value);
//	} else {
//		this->set_field_value(target.second, value);
//	}
//}

/*===----------------  MirrorOop  -------------------===*/
MirrorOop::MirrorOop(shared_ptr<Klass> mirrored_who) : mirrored_who(mirrored_who),
					InstanceOop(std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class"))) {}

/*===----------------  TypeArrayOop  -------------------===*/

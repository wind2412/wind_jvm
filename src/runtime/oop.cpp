/*
 * oop.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/oop.hpp"

/*===----------------  InstanceOop  -----------------===*/
InstanceOop::InstanceOop(shared_ptr<InstanceKlass> klass) : Oop(klass, OopType::_InstanceOop) {
	// alloc non-static-field memory.
	this->field_length = klass->non_static_field_bytes();
	if (this->field_length != 0)
		fields = new uint8_t[this->field_length];
}

unsigned long InstanceOop::get_field_value(int offset, int size) {
	switch (size) {
		case 1:
			return *(uint8_t *)(this->fields + offset);
		case 2:
			return *(uint16_t *)(this->fields + offset);
		case 4:
			return *(uint32_t *)(this->fields + offset);
		case 8:
			return *(uint64_t *)(this->fields + offset);
		default:{
			std::cerr << "can't get here! size == " << size << std::endl;
			assert(false);
		}
	}
}

void InstanceOop::set_field_value(int offset, int size, unsigned long value) {
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

unsigned long InstanceOop::get_value(const wstring & signature) {	// 注意......一个非常重要的问题就是，这里的类型系统是由字节码来提示的：也就是，根据字节码 istore, astore，你可以自动把它们转换为 int/Reference(Oop) 类型......也就是， 这里仅仅返回类型擦除后的 unsigned long(max 8 bytes, 64bits) 就可以。
	// static / non-static 的信息被我给抹去了。外边不知道调用的是 static 还是 non-static.
	auto this_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto target = this_klass->get_field(signature);
	assert(target.first != -1);	// valid field
	if (target.second->is_static()) {
		return this_klass->get_static_field_value(target.first, target.second->get_value_size());
	} else {
		return this->get_field_value(target.first, target.second->get_value_size());
	}
}

void InstanceOop::set_value(const wstring & signature, unsigned long value) {
	auto this_klass = std::static_pointer_cast<InstanceKlass>(this->klass);
	auto target = this_klass->get_field(signature);
	assert(target.first != -1);	// valid field
	if (target.second->is_static()) {
		this_klass->set_static_field_value(target.first, target.second->get_value_size(), value);
	} else {
		this->set_field_value(target.first, target.second->get_value_size(), value);
	}
}

/*===----------------  TypeArrayOop  -------------------===*/











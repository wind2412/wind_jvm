/*
 * sun_misc_Unsafe.cpp
 *
 *  Created on: 2017年11月22日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_Unsafe.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"

static unordered_map<wstring, void*> methods = {
    {L"arrayBaseOffset:(" CLS ")I",				(void *)&JVM_ArrayBaseOffset},
    {L"arrayIndexScale:(" CLS ")I",				(void *)&JVM_ArrayIndexScale},
    {L"addressSize:()I",							(void *)&JVM_AddressSize},
    {L"objectFieldOffset:(" FLD ")J",			(void *)&JVM_ObjectFieldOffset},
    {L"getIntVolatile:(" OBJ "J)I",				(void *)&JVM_GetIntVolatile},
};

void JVM_ArrayBaseOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::wcout << "[arrayBaseOffset] " << _array->get_buf_offset() << std::endl;		// delete
	_stack.push_back(new IntOop(_array->get_buf_offset()));
}
void JVM_ArrayIndexScale(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
	std::wcout << "[arrayScaleOffset] " << sizeof(intptr_t) << std::endl;		// delete
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}
void JVM_AddressSize(list<Oop *> & _stack){
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}
// see: http://hllvm.group.iteye.com/group/topic/37940
void JVM_ObjectFieldOffset(list<Oop *> & _stack){		// 我只希望不要调用这些函数...因为我根本不知道正确的实现是什么.....
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *field = (InstanceOop *)_stack.front();	_stack.pop_front();	// java/lang/reflect/Field obj.

	// 需要 new 一个对象... 实测...				// TODO: 还要支持 static 的！这时不用 new 对象了。
	Oop *oop;
	assert(field->get_field_value(L"name:Ljava/lang/String;", &oop));
	wstring name = java_lang_string::stringOop_to_wstring((InstanceOop *)oop);

	assert(field->get_field_value(L"type:Ljava/lang/Class;", &oop));
	MirrorOop *mirror = (MirrorOop *)oop;

	wstring descriptor = name + L":";

	// get the Type of this member variable.	// maybe: BasicType, InstanceType, ArrayType{ObjArrayType, BasicArrayType}.
	shared_ptr<Klass> mirrored_who = mirror->get_mirrored_who();
	if (mirrored_who) {	// not primitive type
		if (mirrored_who->get_type() == ClassType::InstanceClass) {
			descriptor += (L"L" + mirrored_who->get_name() + L";");
		} else if (mirrored_who->get_type() == ClassType::ObjArrayClass) {
			assert(false);		// TODO: 因为我并不知道怎么写，而且怕写错...
		} else if (mirrored_who->get_type() == ClassType::TypeArrayClass) {
			descriptor += mirrored_who->get_name();
		} else {
			assert(false);		// TODO: 同上...
		}
	} else {
		assert(mirror->get_extra() != L"");
		descriptor += mirror->get_extra();
	}

	// get the class which has the member variable.
	assert(field->get_field_value(L"clazz:Ljava/lang/Class;", &oop));
	MirrorOop *outer_klass_mirror = (MirrorOop *)oop;
	assert(outer_klass_mirror->get_mirrored_who()->get_type() == ClassType::InstanceClass);	// outer must be InstanceType.
	shared_ptr<InstanceKlass> outer_klass = std::static_pointer_cast<InstanceKlass>(outer_klass_mirror->get_mirrored_who());

	// try to new a obj
	int offset = outer_klass->new_instance()->get_field_offset(descriptor);			// TODO: GC!!

#ifdef DEBUG
	std::wcout << "(DEBUG) the field which names [ " << descriptor << " ], inside the [" << outer_klass->get_name() << "], has the offset [" << offset << "] of its FIELDS." << std::endl;
#endif

	_stack.push_back(new LongOop(offset));		// 这时候万一有了 GC，我的内存布局就全都变了...
}

void JVM_GetIntVolatile(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();

#ifdef DEBUG
	std::wcout << "(DEBUG) [dangerous] will get an int from obj oop:[" << obj << "], which klass_name is: [" <<
			std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_name() << "], offset: [" << offset << "]: ";
#endif
	// 非常危险...
	Oop *target = obj->get_fields_addr()[offset];
	assert(target->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)target)->get_type() == Type::INT);
	_stack.push_back(new IntOop(((IntOop *)target)->value));
#ifdef DEBUG
	std::wcout << "int value is [" << ((IntOop *)target)->value << "] " << std::endl;
#endif
}

// 返回 fnPtr.
void *sun_misc_unsafe_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

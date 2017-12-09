/*
 * java_lang_invoke_MethodHandleNatives.cpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_invoke_MethodHandleNatives.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/os.hpp"
#include "classloader.hpp"

static unordered_map<wstring, void*> methods = {
    {L"getConstant:(I)I",							(void *)&JVM_GetConstant},
    {L"resolve:(" MN CLS ")" MN,						(void *)&JVM_Resolve},
    {L"expand:(" MN ")V",							(void *)&JVM_Expand},
};

void JVM_GetConstant(list<Oop *> & _stack){		// static
	int num = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	assert(num == 4);		// 看源码得到的...
	_stack.push_back(new IntOop(false));		// 我就 XJB 返回了......看到源码好像没有用到这的...而且也看不太懂....定义 COMPILER2 宏就 true ???
}

wstring get_full_name(MirrorOop *mirror)
{
	auto klass = mirror->get_mirrored_who();
	if (klass == nullptr) {		// primitive
		return mirror->get_extra();
	} else {
		if (klass->get_type() == ClassType::InstanceClass) {
			return L"L" + klass->get_name() + L";";
		} else if (klass->get_type() == ClassType::ObjArrayClass || klass->get_type() == ClassType::TypeArrayClass){
			return klass->get_name();
		} else {
			assert(false);
		}
	}
}

void JVM_Resolve(list<Oop *> & _stack){		// static
	InstanceOop *member_name_obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	MirrorOop *caller_mirror = (MirrorOop *)_stack.front();	_stack.pop_front();		// I ignored it.

	if (member_name_obj == nullptr) {
		assert(false);		// TODO: throw InternalError
	}

	Oop *oop;
	member_name_obj->get_field_value(MEMBERNAME L":clazz:" CLS, &oop);
	MirrorOop *clazz = (MirrorOop *)oop;		// e.g.: Test8
	assert(clazz != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":name:" STR, &oop);
	InstanceOop *name = (InstanceOop *)oop;	// e.g.: doubleVal
	assert(name != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":type:" OBJ, &oop);
	InstanceOop *type = (InstanceOop *)oop;	// maybe a String, or maybe an Object[]...
	assert(type != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
	int flags = ((IntOop *)oop)->value;

	std::wcout << clazz->get_mirrored_who()->get_name() << " " << java_lang_string::stringOop_to_wstring(name) << std::endl;

	auto klass = clazz->get_mirrored_who();

	// decode is from openjdk:
	int ref_kind = ((flags & 0xF000000) >> 24);
	/**
	 * from 1 ~ 9:
	 * 1: getField
	 * 2: getStatic
	 * 3: putField
	 * 4: putStatic
	 * 5: invokeVirtual
	 * 6: invokeStatic
	 * 7: invokeSpecial
	 * 8: newInvokeSpecial
	 * 9: invokeInterface
	 */
	assert(ref_kind >= 1 && ref_kind <= 9);

	if (klass == nullptr) {
		assert(false);
	} else if (klass->get_type() == ClassType::InstanceClass) {
		// right. do nothing.
	} else if (klass->get_type() == ClassType::ObjArrayClass || klass->get_type() == ClassType::TypeArrayClass) {
		klass = BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Object");		// TODO: 并不清楚为何要这样替换。
	} else {
		assert(false);
	}

	// 0. create a empty wstring: descriptor
	wstring descriptor(L"(");
	// 1. should parse the `Object type;` member first.
	if (type->get_klass()->get_name() == L"java/lang/invoke/MethodType") {
		Oop *oop;
		// 1-a-1: get the args type.
		type->get_field_value(METHODTYPE L":ptypes:[" CLS, &oop);
		assert(oop != nullptr);
		auto class_arr_obj = (ArrayOop *)oop;
		for (int i = 0; i < class_arr_obj->get_length(); i ++) {
			descriptor += get_full_name((MirrorOop *)(*class_arr_obj)[i]);
		}
		descriptor += L")";
		// 1-a-2: get the return type.
		type->get_field_value(METHODTYPE L":rtype:" CLS, &oop);
		assert(oop != nullptr);
		descriptor += get_full_name((MirrorOop *)oop);
	} else if (type->get_klass()->get_name() == L"java/lang/Class") {
		assert(false);
	} else if (type->get_klass()->get_name() == L"java/lang/String") {
		assert(false);
	} else {
		assert(false);
	}

	auto real_klass = std::static_pointer_cast<InstanceKlass>(klass);
	wstring real_name = java_lang_string::stringOop_to_wstring(name);
	if (real_name == L"<clinit>" || real_name == L"<init>") {
		assert(false);		// can't be the two names.
	}
	wstring signature = real_name + L":" + descriptor;

	if (flags & 0x10000) {		// Method:
		shared_ptr<Method> target_method;
		if (ref_kind == 6)	{	// invokeStatic
			target_method = real_klass->get_this_class_method(signature);
			assert(target_method != nullptr);
		} else {
			assert(false);		// not support yet...
		}

		// build the return MemberName obj.
		auto member_name2 = std::static_pointer_cast<InstanceKlass>(member_name_obj->get_klass())->new_instance();
		int new_flag = (target_method->get_flag() & (~ACC_ANNOTATION));
		if (target_method->has_annotation_name_in_method(L"Lsun/reflect/CallerSensitive;")) {
			new_flag |= 0x100000;
		}
		if (ref_kind == 6) {
			new_flag |= 0x10000 | (6 << 24);
		} else {
			assert(false);
		}

		member_name2->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
		member_name2->set_field_value(MEMBERNAME L":name:" STR, name);
		member_name2->set_field_value(MEMBERNAME L":type:" OBJ, type);
		member_name2->set_field_value(MEMBERNAME L":clazz:" CLS, target_method->get_klass()->get_mirror());
		_stack.push_back(member_name2);
		return;
	} else {
		assert(false);			// not support yet...
	}



	assert(false);
}

void JVM_Expand(list<Oop *> & _stack) {
	// 这个方法无法使用。因为我的实现没有 itable...而且我也并不知道 vmindex 应该被放到哪里。
}



// 返回 fnPtr.
void *java_lang_invoke_methodHandleNatives_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


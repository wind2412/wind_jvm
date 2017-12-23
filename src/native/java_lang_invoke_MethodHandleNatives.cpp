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
#include "wind_jvm.hpp"
#include <algorithm>

Lock member_name_table_lock;
set<InstanceOop *> member_name_table;

InstanceOop *find_table_if_match_methodType(InstanceOop *methodType)		// find in the `member_name_table` to match for the `methodType`, for `invoke()`...
{
	LockGuard lg(member_name_table_lock);

	auto predicate = [&methodType](InstanceOop *member_name_obj) -> bool {
		assert(member_name_obj != nullptr);
		Oop *oop;
		member_name_obj->get_field_value(MEMBERNAME L":type:" OBJ, &oop);		// get inner `methodType` in `member_name_obj`.
		if (oop == methodType) {
			return true;
		}
		return false;
	};

	// become O(N)...
	auto iter = std::find_if(member_name_table.begin(), member_name_table.end(), predicate);
	if (iter == member_name_table.end()) {
		assert(false);		// can't be `didn't find`.
	}
	return *iter;
}

static unordered_map<wstring, void*> methods = {
    {L"getConstant:(I)I",												(void *)&JVM_GetConstant},
    {L"resolve:(" MN CLS ")" MN,											(void *)&JVM_Resolve},
    {L"expand:(" MN ")V",												(void *)&JVM_Expand},
    {L"init:(" MN OBJ ")V",												(void *)&JVM_Init},
    {L"objectFieldOffset:(" MN ")J",										(void *)&JVM_MH_ObjectFieldOffset},
    {L"getMembers:(" CLS STR STR "I" CLS "I" "[" MN ")I",					(void *)&JVM_GetMembers},
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

InstanceOop *fill_in_MemberName_with_Method(shared_ptr<Method> target_method, InstanceOop *origin_member_name_obj, int ref_kind, Oop *type, vm_thread *thread = nullptr)	// vm_thread: for debug
{
	auto member_name2 = ((InstanceKlass *)origin_member_name_obj->get_klass())->new_instance();
	int new_flag = (target_method->get_flag() & (~ACC_ANNOTATION));
	if (target_method->has_annotation_name_in_method(L"Lsun/reflect/CallerSensitive;")) {
		new_flag |= 0x100000;
	}

	assert(ref_kind >= 5 && ref_kind <= 9);
	new_flag |= 0x10000 | (ref_kind << 24);		// invokeStatic

//	// delete all for debug
//	std::wcout << target_method->get_name() << " " << type << std::endl;
//	if (thread != nullptr) {
//		auto toString = ((InstanceKlass *)type->get_klass())->get_this_class_method(L"toString:()" STR);
//		assert(toString != nullptr);
//		InstanceOop *str = (InstanceOop *)thread->add_frame_and_execute(toString, {type});
//		std::wcout << java_lang_string::stringOop_to_wstring(str) << std::endl;
//		std::wcout << type << std::endl;
//	}

	member_name2->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
	member_name2->set_field_value(MEMBERNAME L":name:" STR, java_lang_string::intern(target_method->get_name()));
	member_name2->set_field_value(MEMBERNAME L":type:" OBJ, type);
	member_name2->set_field_value(MEMBERNAME L":clazz:" CLS, target_method->get_klass()->get_mirror());

	LockGuard lg(member_name_table_lock);
	member_name_table.insert(member_name2);		// save to the Table
	member_name_table.insert(origin_member_name_obj);		// save to the Table

	return member_name2;
}

InstanceOop *fill_in_MemberName_with_Fieldinfo(shared_ptr<Field_info> target_field, InstanceOop *origin_member_name_obj, int ref_kind)
{
	// build the return MemberName obj.
	auto member_name2 = ((InstanceKlass *)origin_member_name_obj->get_klass())->new_instance();
	int new_flag = (target_field->get_flag() & (~ACC_ANNOTATION));
	if (target_field->is_static()) {
		new_flag |= 0x40000 | (2 << 24);		// getStatic(2)
	} else {
		new_flag |= 0x40000 | (1 << 24);		// getField(1)
	}

	if (ref_kind > 2) {		// putField(3) / putStatic(2)
		new_flag += ((3 - 1) << 24);
	}

	member_name2->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
	member_name2->set_field_value(MEMBERNAME L":name:" STR, java_lang_string::intern(target_field->get_name()));		// will not be settled by Java JDK!! // 必须要设置......
	member_name2->set_field_value(MEMBERNAME L":type:" OBJ, java_lang_string::intern(target_field->get_descriptor()));		// 必须要设置......
//		member_name2->set_field_value(MEMBERNAME L":type:" OBJ, target_field->get_type_klass()->get_mirror());
	member_name2->set_field_value(MEMBERNAME L":clazz:" CLS, target_field->get_klass()->get_mirror());		// set the field's inner klass!!! not the Type!!

	LockGuard lg(member_name_table_lock);
	member_name_table.insert(member_name2);		// save to the Table
	member_name_table.insert(origin_member_name_obj);		// save to the Table

	return member_name2;
}

wstring get_member_name_descriptor(InstanceKlass *real_klass, const wstring & real_name, InstanceOop *type)
{
	wstring descriptor;
	// 0.5. if we should 钦定 these blow: only for real_klass is `java/lang/invoke/MethodHandle`:
	if (real_klass->get_name() == L"java/lang/invoke/MethodHandle" &&
				(real_name == L"invoke"
				|| real_name == L"invokeBasic"				// 钦定这些。因为 else 中都是通过把 ptypes 加起来做到的。但是 MethodHandle 中的变长参数是一个例外。其他类中的变长参数没问题，因为最后编译器会全部转为 Object[]。只有 MethodHandle 是例外～
				|| real_name == L"invokeExact"
				|| real_name == L"invokeWithArauments"
				|| real_name == L"linkToSpecial"
				|| real_name == L"linkToStatic"
				|| real_name == L"linkToVirtual"
				|| real_name == L"linkToInterface"))  {		// 悲伤。由于历史原因（，我的查找是通过字符串比对来做的......简直无脑啊......这样这里效率好低吧QAQ。不过毕竟只是个玩具，跑通就好......
		descriptor = L"([Ljava/lang/Object;)Ljava/lang/Object;";
	} else {
		// 1. should parse the `Object type;` member first.
		if (type->get_klass()->get_name() == L"java/lang/invoke/MethodType") {
			descriptor += L"(";
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
			auto real_klass = ((MirrorOop *)type)->get_mirrored_who();
			if (real_klass == nullptr) {
				descriptor += ((MirrorOop *)type)->get_extra();
			} else {
				if (real_klass->get_type() == ClassType::InstanceClass) {
					descriptor += (L"L" + real_klass->get_name() + L";");
				} else if (real_klass->get_type() == ClassType::TypeArrayClass || real_klass->get_type() == ClassType::ObjArrayClass) {
					descriptor += real_klass->get_name();
				} else {
					assert(false);
				}
			}
		} else if (type->get_klass()->get_name() == L"java/lang/String") {
			assert(false);		// not support yet...
		} else {
			assert(false);
		}
	}
	return descriptor;
}

shared_ptr<Method> get_member_name_target_method(InstanceKlass *real_klass, const wstring & signature, int ref_kind, vm_thread *thread)
{
	shared_ptr<Method> target_method;
	if (ref_kind == 6)	{			// invokeStatic
//		std::wcout << real_klass->get_name() << " " << signature << std::endl;	// delete
		target_method = real_klass->get_this_class_method(signature);
//		assert(target_method != nullptr);
	} else if (ref_kind == 5) { 		// invokeVirtual
		target_method = real_klass->search_vtable(signature);
		assert(target_method != nullptr);
	} else if (ref_kind == 7) {		// invokeSpecial
		assert(false);		// not support yet...

	} else if (ref_kind == 9) {		// invokeInterface
		assert(false);		// not support yet...

	} else {
		assert(false);
	}
	return target_method;		// 可以返回 nullptr ！！！这个千万注意！！像是 jdk 的 resolveOrNull 方法，就是这样，故意找不到然后抛一个 LinkageError，然后捕获！！
}

void JVM_Resolve(list<Oop *> & _stack){		// static
	InstanceOop *member_name_obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	MirrorOop *caller_mirror = (MirrorOop *)_stack.front();	_stack.pop_front();		// I ignored it.
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();

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
			// type 这个变量的类型可能是 String, Class, MethodType!
	assert(type != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
	int flags = ((IntOop *)oop)->value;

//	std::wcout << clazz->get_mirrored_who()->get_name() << " " << java_lang_string::stringOop_to_wstring(name) << std::endl;		// delete

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

	auto real_klass = ((InstanceKlass *)klass);
	wstring real_name = java_lang_string::stringOop_to_wstring(name);
	if (real_name == L"<clinit>" || real_name == L"<init>") {
		assert(false);		// can't be the two names.
	}

	// 0. create a empty wstring: descriptor
	wstring descriptor = get_member_name_descriptor(real_klass, real_name, type);

//	std::wcout << ".....signature: [" << real_klass->get_name() << " " << real_name << " " << descriptor << std::endl;	// delete
//	vm_thread *thread = (vm_thread *)_stack.back();		// delete
//	thread->get_stack_trace();			// delete

	if (flags & 0x10000) {		// Method:

		wstring signature = real_name + L":" + descriptor;
		shared_ptr<Method> target_method = get_member_name_target_method(real_klass, signature, ref_kind, thread);

		if (target_method == nullptr) {		// throw LinkageError !!! Purposely!!!

			// get the exception klass
			auto excp_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/LinkageError"));
			// make a message
			std::wstring msg(L"didn't find the target_method: [" + signature + L"], by wind2412");
			// go!
			native_throw_Exception(excp_klass, thread, _stack, msg);	// the exception obj has been save into the `_stack`!

			return;

		} else {
			// build the return MemberName obj.
			auto member_name2 = fill_in_MemberName_with_Method(target_method, member_name_obj, ref_kind, type, thread);
			_stack.push_back(member_name2);
		}


	} else if (flags & 0x20000){		// Constructor
		assert(false);			// not support yet...
	} else if (flags & 0x40000) {	// Field
		wstring signature = real_name + L":" + descriptor;
		auto _pair = real_klass->get_field(signature);
		assert(_pair.second != nullptr);
		auto target_field = _pair.second;

		auto member_name2 = fill_in_MemberName_with_Fieldinfo(target_field, member_name_obj, ref_kind);

		_stack.push_back(member_name2);
		return;
	} else {
		assert(false);
	}

}


void JVM_Expand(list<Oop *> & _stack) {
	// 这个方法无法使用。因为我的实现没有 itable...而且我也并不知道 vmindex 应该被放到哪里。
}

void JVM_Init(list<Oop *> & _stack){		// static
	InstanceOop *member_name_obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *target = (InstanceOop *)_stack.front();	_stack.pop_front();			// java/lang/Object
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();

	/**
	 * in fact, `target` will be one of the three:
	 * 1. java/lang/reflect/Constructor.
	 * 2. java/lang/reflect/Field.
	 * 3. java/lang/reflect/Method.
	 */
	assert(member_name_obj != nullptr);
	assert(target != nullptr);
	auto klass = ((InstanceKlass *)target->get_klass());


	if (klass->get_name() == L"java/lang/reflect/Constructor") {
		Oop *oop;
		target->get_field_value(CONSTRUCTOR L":slot:I", &oop);
		int slot = ((IntOop *)oop)->value;
		shared_ptr<Method> target_method = klass->search_method_in_slot(slot);

		int new_flag = (target_method->get_flag() & (~ACC_ANNOTATION));
		if (target_method->has_annotation_name_in_method(L"Lsun/reflect/CallerSensitive;")) {
			new_flag |= 0x100000;
		}

		new_flag |= 0x10000 | (7 << 24);		// invokeSpecial: 7

		member_name_obj->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
//		member_name_obj->set_field_value(MEMBERNAME L":name:" STR, name);		// in JDK source code: MemberName::public MemberName(Field fld, boolean makeSetter), the `name` and `type` are settled by the java source code.
//		member_name_obj->set_field_value(MEMBERNAME L":type:" OBJ, type);
		member_name_obj->set_field_value(MEMBERNAME L":clazz:" CLS, klass->get_mirror());

	} else if (klass->get_name() == L"java/lang/reflect/Field") {
		Oop *oop;
		target->get_field_value(FIELD L":modifiers:I", &oop);
		int new_flag = ((IntOop *)oop)->value;
		target->get_field_value(FIELD L":clazz:" CLS, &oop);
		Oop *clazz = oop;

		new_flag = (new_flag & (~ACC_ANNOTATION));

		member_name_obj->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
//		member_name_obj->set_field_value(MEMBERNAME L":name:" STR, name);
//		member_name_obj->set_field_value(MEMBERNAME L":type:" OBJ, type);
		member_name_obj->set_field_value(MEMBERNAME L":clazz:" CLS, clazz);

	} else if (klass->get_name() == L"java/lang/reflect/Method") {
		Oop *oop;
		target->get_field_value(METHOD L":slot:I", &oop);
		int slot = ((IntOop *)oop)->value;
		shared_ptr<Method> target_method = klass->search_method_in_slot(slot);

		int new_flag = (target_method->get_flag() & (~ACC_ANNOTATION));
		if (target_method->has_annotation_name_in_method(L"Lsun/reflect/CallerSensitive;")) {
			new_flag |= 0x100000;
		}

		if (target_method->is_private() && !target_method->is_static()) {		// use invokeSpecial.
			new_flag |= 0x10000 | (7 << 24);		// invokeSpecial: 7
		} else if (target_method->is_static()) {
			new_flag |= 0x10000 | (6 << 24);		// invokeStatic: 6
		} else {
			if (klass->is_in_vtable(target_method)) {
				new_flag |= 0x10000 | (5 << 24);		// invokeVirtual: 5
			} else {
				new_flag |= 0x10000 | (9 << 24);		// invokeInterface: 9
			}
		}

		member_name_obj->set_field_value(MEMBERNAME L":flags:I", new IntOop(new_flag));
//		member_name_obj->set_field_value(MEMBERNAME L":name:" STR, name);		// in JDK source code: MemberName::public MemberName(Field fld, boolean makeSetter), the `name` and `type` are settled by the java source code.
//		member_name_obj->set_field_value(MEMBERNAME L":type:" OBJ, type);
		member_name_obj->set_field_value(MEMBERNAME L":clazz:" CLS, klass->get_mirror());


		// 这里要进行魔改一下。由于在 invokeBasic 那里碰到了问题，导致搜索 MethodType 搜索不到————原因有可能是：因为这里没有设置 type，因此 jdk 设置了。在 Method 那里，
		// 有几率会被设置成 Object[]...... 而不是 MethodType。现在很好奇是不是这个原因......于是，在这里测试一发。
		// 唉...... 看来是失败了。不过代码还是留下吧。

/*
		auto method_type_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/invoke/MethodType"));
		assert(method_type_klass != nullptr);
		auto init_method_type = method_type_klass->get_this_class_method(L"methodType:(" CLS "[" CLS ")" MT);		// static method!!!
		assert(init_method_type != nullptr);

		auto arg_list_vector = target_method->parse_argument_list();		// vector<MirrorOop *>
		auto return_mirror = target_method->parse_return_type();			// MirrorOop *
		if (return_mirror->get_extra() == L"V") {			// 卧槽！！设置 MethodType.type 的时候，返回值如果是 void，也要强行变成 java/lang/Void!!!
			return_mirror = BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Void")->get_mirror();
		}

		auto klass_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[" CLS));
		assert(klass_arr_klass != nullptr);
		auto klass_arr_obj = klass_arr_klass->new_instance(arg_list_vector.size());		// alloc a [Ljava/lang/Class; for arg list's length.

		// fill in
		for (int i = 0; i < arg_list_vector.size(); i ++) {
			(*klass_arr_obj)[i] = arg_list_vector[i];
		}

		// run <init>!
		InstanceOop *method_type_obj = (InstanceOop *)thread->add_frame_and_execute(init_method_type, {return_mirror, klass_arr_obj});

//		// delete all for debug
//		std::wcout << target_method->get_name() << " " << method_type_obj << std::endl;
//		if (thread != nullptr) {
//			auto toString = ((InstanceKlass *)method_type_obj->get_klass())->get_this_class_method(L"toString:()" STR);
//			assert(toString != nullptr);
//			InstanceOop *str = (InstanceOop *)thread->add_frame_and_execute(toString, {method_type_obj});
//			std::wcout << java_lang_string::stringOop_to_wstring(str) << std::endl;
//			std::wcout << method_type_obj << std::endl;
//		}

		member_name_obj->set_field_value(MEMBERNAME L":type:" OBJ, method_type_obj);
*/
	} else {
		assert(false);
	}

	LockGuard lg(member_name_table_lock);
	member_name_table.insert(member_name_obj);		// save to the Table

}

void JVM_MH_ObjectFieldOffset(list<Oop *> & _stack){		// static		// 由一个 MN (field) 得到 field 在 klass 中的偏移量。
	InstanceOop *member_name_obj = (InstanceOop *)_stack.front();	_stack.pop_front();

	assert(member_name_table.find(member_name_obj) != member_name_table.end());		// simple check...

	assert(member_name_obj != nullptr);
	// for check:
	Oop *oop;
	member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
	int flags = ((IntOop *)oop)->value;
	assert((flags & 0x40000) != 0);		// is a field.
	member_name_obj->get_field_value(MEMBERNAME L":clazz:" CLS, &oop);
	MirrorOop *clazz = (MirrorOop *)oop;		// the fields
	assert(clazz != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":name:" STR, &oop);
	InstanceOop *name = (InstanceOop *)oop;
	assert(name != nullptr);
	member_name_obj->get_field_value(MEMBERNAME L":type:" OBJ, &oop);
	InstanceOop *type = (InstanceOop *)oop;	// type 这个变量的类型可能是 String, Class, MethodType!	// 如果此 MN 表示 Field 的话，按照上边 JVM_Resolve 中对 Field 的设置，type 是字符串.... 不过实测确实 Class...... 看来是 Java 给替换掉了...
	assert(type != nullptr);

	assert(type->get_klass()->get_name() == L"java/lang/Class");		// 后边被 Java 设置成了 Class。

	assert(clazz->get_mirrored_who() != nullptr);
	auto real_klass = ((InstanceKlass *)clazz->get_mirrored_who());		// klass
	wstring real_name = java_lang_string::stringOop_to_wstring(name);							// name
	wstring fake_descriptor = ((MirrorOop *)type)->get_mirrored_who()->get_name();					// [x] descriptor
	if (fake_descriptor[0] != L'[') {		// InstanceKlass
		fake_descriptor = (L"L" + fake_descriptor + L";");
	}
	int offset = real_klass->get_all_field_offset(real_klass->get_name() + L":" + real_name + L":" + fake_descriptor);
//	std::wcout << real_klass->get_name() << ":" << real_name << ":" << fake_descriptor << " " << offset << std::endl;

	_stack.push_back(new LongOop(offset));

	LockGuard lg(member_name_table_lock);
	member_name_table.insert(member_name_obj);		// save to the Table
}

void JVM_GetMembers(list<Oop *> & _stack) {		// static // 整个 Java8 只有一处：getMembers 用到了 MethodHandleNatives.getMembers。
	MirrorOop *klass = (MirrorOop *)_stack.front();	_stack.pop_front();
	InstanceOop *match_name = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *match_sig = (InstanceOop *)_stack.front();	_stack.pop_front();
	int match_flag = ((IntOop *)_stack.front())->value;	_stack.pop_front();			// the **REAL** flag!! no encode!!
	MirrorOop *caller_mirror = (MirrorOop *)_stack.front();	_stack.pop_front();		// of no use.
	int skip = ((IntOop *)_stack.front())->value;	_stack.pop_front();					// of no use.
	ObjArrayOop *member_name_arr = (ObjArrayOop *)_stack.front();	_stack.pop_front();	// this array is initialized in Java8, fully by dummy structures.

	assert(klass != nullptr && member_name_arr != nullptr);
	for (int i = 0; i < member_name_arr->get_length(); i ++) {
		assert((*member_name_arr)[i] != nullptr);		// 因为只有一处使用，而那里是直接 fully initialized by `new` dummy structures 的，所以我直接这么判定了。
	}
	assert(klass->get_mirrored_who() != nullptr);

	bool search_super_klass = ((match_flag & 0x100000) != 0);
	bool search_interfaces  = ((match_flag & 0x200000)   != 0);

	auto real_klass = ((InstanceKlass *)klass->get_mirrored_who());
	auto caller_klass = caller_mirror != nullptr ? caller_mirror->get_mirrored_who() : nullptr;
	wstring real_name = match_name == nullptr ? L"" : java_lang_string::stringOop_to_wstring(match_name);
	wstring real_signature = match_sig == nullptr ? L"" : java_lang_string::stringOop_to_wstring(match_sig);

	if (match_flag & 0x10000) {				// Field
		if (real_name != L"" && real_signature != L"") {	// 指定了某个固定的 name 和 signature。这样直接查找即可。
			wstring descriptor = real_name + L":" + real_signature;		// TODO: 就是不知道这个 signature 到底是 Ljava/lang/Object; 还是 java/lang/Object...
			auto _pair = real_klass->get_field(descriptor);
			assert(_pair.second != nullptr);
			fill_in_MemberName_with_Fieldinfo(_pair.second, (InstanceOop*)(*member_name_arr)[0], 0);	// 第三个参数只要小于 2 即可。即定义为 getField / getStatic，而不是 put。
		} else {
			vector<shared_ptr<Field_info>> v;
			if (!search_super_klass && !search_interfaces) {
				for (auto field : real_klass->get_field_layout()) {
					v.push_back(field.second.second);
				}
				for (auto field : real_klass->get_static_field_layout()) {
					v.push_back(field.second.second);
				}
			} else {
				assert(false);		// not support yet...
			}

			// fill in:
			int len = std::min((int)v.size(), member_name_arr->get_length());
			for (int i = 0; i < len; i ++) {
				fill_in_MemberName_with_Fieldinfo(v[i], (InstanceOop*)(*member_name_arr)[i], 0);
			}
			_stack.push_back(new IntOop(v.size()));		// 总共应该填进去的 nums 数量。Jdk 接到这个数字，判断之后会看数组是不是分配太小，会重新分配之后引发此函数重新进行。
		}
	} else if (match_flag & 0x20000) {		// Constructor
		assert(false);		// not support yet...

	} else if (match_flag & 0x40000) {		// Method
		assert(false);		// not support yet...

	} else {
		assert(false);
	}

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


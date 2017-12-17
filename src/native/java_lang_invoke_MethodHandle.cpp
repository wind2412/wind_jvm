/*
 * java_lang_invoke_MethodHandle.cpp
 *
 *  Created on: 2017年12月13日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_invoke_MethodHandle.hpp"
#include "native/java_lang_invoke_MethodHandleNatives.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/os.hpp"
#include "classloader.hpp"
#include "wind_jvm.hpp"

static unordered_map<wstring, void*> methods = {
    {L"invoke:([" OBJ ")" OBJ,								(void *)&JVM_Invoke},
    {L"invokeBasic:([" OBJ ")" OBJ,							(void *)&JVM_InvokeBasic},
    {L"invokeExact:([" OBJ ")" OBJ,							(void *)&JVM_InvokeExact},
};

// args 是所有的参数，包括 this。而 method->parse_argument_list 是不包括 this 的。
void argument_unboxing(shared_ptr<Method> method, list<Oop *> & args)		// Unboxing args for Integer, Double ... to int, double, [automatically] etc.
{
	vector<MirrorOop *> real_arg_mirrors = method->parse_argument_list();
	// check
	if (method->is_static()) {
		assert(args.size() == real_arg_mirrors.size());
	} else {
		assert(args.size() == real_arg_mirrors.size() + 1);
	}
	list<Oop *> temp;
	// first we should check `this`
	if (!method->is_static()) {	// jump over `this`
		Oop *oop = args.front();	args.pop_front();
		assert(oop != nullptr && oop->get_ooptype() == OopType::_InstanceOop);
		temp.push_back(oop);
	}
	// then other args.
	int i = 0;
	for (Oop *oop : args) {
		MirrorOop *mirror = real_arg_mirrors.at(i++);
		if (oop == nullptr) {		// this circumstance must be for InstanceKlass.
			assert(mirror->get_mirrored_who() != nullptr);
			temp.push_back(nullptr);
			continue;
		} else {						// can be all.
			InstanceOop *real_oop = (InstanceOop *)oop;
			wstring klass_name = oop->get_klass()->get_name();
			Oop *ret;
			if (mirror->get_mirrored_who() == nullptr) {		// primitive type really. need to be unboxing.
				if (klass_name == BYTE0) {
					real_oop->get_field_value(BYTE0 L":value:B", &ret);
					temp.push_back(new IntOop(((IntOop *)ret)->value));
				} else if (klass_name == BOOLEAN0) {
					real_oop->get_field_value(BOOLEAN0 L":value:Z", &ret);
					temp.push_back(new IntOop(((IntOop *)ret)->value));
				} else if (klass_name == SHORT0) {
					real_oop->get_field_value(SHORT0 L":value:S", &ret);
					temp.push_back(new IntOop(((IntOop *)ret)->value));
				} else if (klass_name == CHARACTER0) {
					real_oop->get_field_value(CHARACTER0 L":value:C", &ret);
					temp.push_back(new IntOop(((IntOop *)ret)->value));
				} else if (klass_name == INTEGER0) {
					real_oop->get_field_value(INTEGER0 L":value:I", &ret);
					temp.push_back(new IntOop(((IntOop *)ret)->value));
				} else if (klass_name == FLOAT0) {
					real_oop->get_field_value(FLOAT0 L":value:F", &ret);
					temp.push_back(new FloatOop(((FloatOop *)ret)->value));
				} else if (klass_name == DOUBLE0) {
					real_oop->get_field_value(DOUBLE0 L":value:D", &ret);
					temp.push_back(new DoubleOop(((DoubleOop *)ret)->value));
				} else if (klass_name == LONG0) {
					real_oop->get_field_value(LONG0 L":value:J", &ret);
					temp.push_back(new LongOop(((LongOop *)ret)->value));
				} else {
					std::wcout << klass_name << std::endl;
					assert(false);
				}
			} else {
				temp.push_back(real_oop);
			}
		}
	}
	temp.swap(args);
}

InstanceOop *return_val_boxing(Oop *basic_type_oop, vm_thread *thread, const wstring & return_type)	// $3 is prevent from returning `void`. I think it should be boxed to `Void`.
{
	if (return_type == L"V") {		// TODO: 并不确保正确......应当试验一番......
		return std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Void"))->new_instance();
	}

	if (basic_type_oop == nullptr)	return nullptr;
	if (basic_type_oop->get_ooptype() != OopType::_BasicTypeOop)	return (InstanceOop *)basic_type_oop;		// it is an InstanceOop......	// bug report... 我是白痴......


	shared_ptr<Method> target_method;
	switch (((BasicTypeOop *)basic_type_oop)->get_type()) {
		case Type::BOOLEAN: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(BOOLEAN0));
			target_method = box_klass->get_this_class_method(L"valueOf:(Z)Ljava/lang/Boolean;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::BYTE: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(BYTE0));
			target_method = box_klass->get_this_class_method(L"valueOf:(B)Ljava/lang/Byte;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::SHORT: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(SHORT0));
			target_method = box_klass->get_this_class_method(L"valueOf:(S)Ljava/lang/Short;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::CHAR: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(CHARACTER0));
			target_method = box_klass->get_this_class_method(L"valueOf:(C)Ljava/lang/Character;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::INT: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(INTEGER0));
			target_method = box_klass->get_this_class_method(L"valueOf:(I)Ljava/lang/Integer;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::FLOAT: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(FLOAT0));
			target_method = box_klass->get_this_class_method(L"valueOf:(F)Ljava/lang/Float;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new FloatOop(((FloatOop *)basic_type_oop)->value)});
		}
		case Type::LONG: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(LONG0));
			target_method = box_klass->get_this_class_method(L"valueOf:(J)Ljava/lang/Long;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new LongOop(((LongOop *)basic_type_oop)->value)});
		}
		case Type::DOUBLE: {
			auto box_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(DOUBLE0));
			target_method = box_klass->get_this_class_method(L"valueOf:(D)Ljava/lang/Double;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new DoubleOop(((DoubleOop *)basic_type_oop)->value)});
		}
		default:
			assert(false);
	};
}

Oop *invoke(InstanceOop *member_name_obj, list<Oop *> & _stack, vm_thread *thread)		// _stack 在这里，必须**全是**invoke 方法的参数！！不能有其他的杂质！！
{
	Oop *oop;
	// 非常悲伤。因为研究了好长时间也没有研究出来那个 vmindex 和 vmtarget 到底放在哪。感觉应该是 jvm 对 MemberName 这个类钦定的吧......
	// 所以这里只能重新查找了......QAQ
	// 所以下边的代码调用了 JVM_Resolve 的大部分函数......
	// 在每次解析出来 MemberName 的时候，都要把它放到一个 Table 中！！这个 MethodName 中自带一个 MethodType，这个 MethodType 的对象或许能用......
	// 应该可以和 MethodHandle 中的 MethodType 查地址然后配对......(已采用)

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
	assert(ref_kind >= 5 && ref_kind <= 9);		// must be method call...

	auto real_klass = std::static_pointer_cast<InstanceKlass>(klass);
	wstring real_name = java_lang_string::stringOop_to_wstring(name);
	if (real_name == L"<clinit>" || real_name == L"<init>") {
		assert(false);		// can't be the two names.
	}

	// 0. create a empty wstring: descriptor
	wstring descriptor = get_member_name_descriptor(real_klass, real_name, type);
	// 1. get the signature
	wstring signature = real_name + L":" + descriptor;
	// 2. get the target method
	shared_ptr<Method> target_method = get_member_name_target_method(real_klass, signature, ref_kind);

	// simple check
	int size = target_method->parse_argument_list().size();
	// add `this` obj!
	if (ref_kind == 6)	{			// invokeStatic

	} else if (ref_kind == 5) { 		// invokeVirtual
		size ++;
	} else if (ref_kind == 7) {		// invokeSpecial
		size ++;
		assert(false);		// not support yet...
	} else if (ref_kind == 9) {		// invokeInterface
		size ++;
		assert(false);		// not support yet...
	} else {
		size ++;
		assert(false);
	}
	assert(size == _stack.size()
		   || (_stack.size() == 1 && _stack.front()->get_klass()->get_name() == L"[Ljava/lang/Object;"));		// 参数一定要相等......

	// 如果是后者，即没有解包的 [Object，那么就解包。
	if (size != _stack.size()) {		// 这一句同时也防止了 invoke 的参数真的是 [Object 的情况，然后还 xjb 解包。
		ObjArrayOop *obj_arr = (ObjArrayOop *)_stack.front();	_stack.pop_front();
		assert(obj_arr->get_length() == size);		// 这回一定相等了。
		for (int i = 0; i < obj_arr->get_length(); i ++) {	// 解包之后完全压入！
			_stack.push_back((*obj_arr)[i]);
		}
	}

	// 把参数所有的自动装箱类解除装箱...... 因为 invoke 的参数全是 Object，而真实的参数可能是 int。
	argument_unboxing(target_method, _stack);		// ...... 万一参数真是 Integer 呢...... 我在这个方法里解决了。

	// 3. call it!		// return maybe: BasicTypeOop, ArrayOop, InstanceOop... all.
	Oop *result = thread->add_frame_and_execute(target_method, _stack);		// TODO: 如果抛了异常。

	// 把返回值所有能自动装箱的自动装箱......因为要返回一个 Object。
	Oop *real_result = return_val_boxing(result, thread, target_method->return_type());

	return real_result;
}

void JVM_Invoke(list<Oop *> & _stack){
	// 注意！！接下来是硬编码！因为 _stack 最后两个，为了防止 native 函数内部还要调用 java 方法，以及在 static native 方法得到调用者的 klass，我当时在 _stack 的后边默认加了两个参数：
	// 即，_stack 的内存模型应该是这样：(假设长度为 length，即下标范围: [0~length-1])
	// [0]. _this(如果此方法为 native non-static 的话，有此 _this 参数)
	// [1]. Arg0
	// [2]. Arg1
	//	...
	// [length-2]: CallerKlassMirror *
	// [length-1]: vm_thread * (此 native method 所在的线程栈)
	// 之所以会有后两个，就是因为以上原因。
	// 而这里，由于参数不定长度，因此必须计算出来 _stack.size()，然后 -2，再 -1 (减去 _this)，即是所有参数的长度。
	// 而日后不知道会不会需要改进，而在 [length-3] 处放置 Caller 的其他信息。
	// 所以特此留下说明。届时需要把 _stack.size() - 2 - 1 改成 _stack.size() - 3 - 1, etc.

	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop 出 [0] 的 _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop 出 [length-1] 的 vm_thread
	_stack.pop_back();															// pop 出 [length-2] 的 CallerKlassMirror *.
	// 现在 _stack 剩下的全是参数～。

	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

	// get the **REAL MemberName** from Table through methodType...
	InstanceOop *member_name_obj = find_table_if_match_methodType(methodType);		// TODO: 其实如果是 DirectMethodHandle，可以直接查找......貌似不用 find_table...

	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);
}

void JVM_InvokeBasic(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop 出 [0] 的 _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop 出 [length-1] 的 vm_thread
	_stack.pop_back();															// pop 出 [length-2] 的 CallerKlassMirror *.
	// 现在 _stack 剩下的全是参数～。

	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

//	// delete all for debug (toString())
//	auto toString = std::static_pointer_cast<InstanceKlass>(methodType->get_klass())->get_this_class_method(L"toString:()" STR);
//	assert(toString != nullptr);
//	InstanceOop *str = (InstanceOop *)thread->add_frame_and_execute(toString, {methodType});
//	std::wcout << java_lang_string::stringOop_to_wstring(str) << std::endl;
//	std::wcout << methodType << std::endl;

//	methodType->get_field_value(METHODTYPE L":methodDescriptor:" STR, &oop);
//	if (oop != nullptr)
//		std::wcout << java_lang_string::stringOop_to_wstring((InstanceOop *)oop) << std::endl;

//	// get the **REAL MemberName** from Table through methodType...
//	InstanceOop *member_name_obj = find_table_if_match_methodType(methodType);	// 根本就没有被传进来.....需要另想办法了......

	// 这也就是说明，调用到这里，虽然有 MethodType，但是却并找不到对应的 MemberName ???!!! 怎么回事到底是？？？？

	assert(_this->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");		// DirectMethodHandle 中直接包含了一个 MemberName 项！！

	_this->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
	InstanceOop *member_name_obj = (InstanceOop *)oop;

	// invokeBasic 很特殊：因为包括 this 的参数统统压入了 _stack，因此并不需要像 invokeExact 一样，需要判断 ref_kind 并多压入一个参数。

	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);

}

wstring toString(InstanceOop *oop, vm_thread *thread)		// for debugging
{
	auto real_klass = std::static_pointer_cast<InstanceKlass>(oop->get_klass());
	auto toString = real_klass->search_vtable(L"toString:()Ljava/lang/String;");	// don't use `find_in_this_klass()..."
	assert(toString != nullptr);
	InstanceOop *str = (InstanceOop *)thread->add_frame_and_execute(toString, {oop});
	return java_lang_string::print_stringOop(str);
}

void JVM_InvokeExact(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop 出 [0] 的 _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop 出 [length-1] 的 vm_thread
	_stack.pop_back();															// pop 出 [length-2] 的 CallerKlassMirror *.
	// 现在 _stack 剩下的全是参数～。

	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

//	// delete all for debug (toString())
//	auto toString = std::static_pointer_cast<InstanceKlass>(methodType->get_klass())->get_this_class_method(L"toString:()" STR);
//	assert(toString != nullptr);
//	InstanceOop *str = (InstanceOop *)thread->add_frame_and_execute(toString, {methodType});
//	std::wcout << java_lang_string::stringOop_to_wstring(str) << std::endl;
//	std::wcout << methodType << std::endl;

//	methodType->get_field_value(METHODTYPE L":methodDescriptor:" STR, &oop);
//	if (oop != nullptr)
//		std::wcout << java_lang_string::stringOop_to_wstring((InstanceOop *)oop) << std::endl;

//	// get the **REAL MemberName** from Table through methodType...
//	InstanceOop *member_name_obj = find_table_if_match_methodType(methodType);	// 根本就没有被传进来.....需要另想办法了......

	// 这也就是说明，调用到这里，虽然有 MethodType，但是却并找不到对应的 MemberName ???!!! 怎么回事到底是？？？？

//	assert(_this->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");		// DirectMethodHandle 中直接包含了一个 MemberName 项！！


	InstanceOop *member_name_obj;

	// `this` may be: `java/lang/invoke/BoundMethodHandle$Species_LL`.
	if (_this->get_klass()->get_name() == L"java/lang/invoke/BoundMethodHandle$Species_LL") {
		_this->get_field_value(SPECIES_LL ":argL0:" OBJ, &oop);		// sample: klass:[DirectMethodHandle], toString():[MethodHandle(PrintStream,String)void]
		auto argL0 = oop;
		_this->get_field_value(SPECIES_LL ":argL1:" OBJ, &oop);		// sample: klass:[PrintStream]		( the final argument is in `_stack` ).
		auto argL1 = oop;

		assert(argL0->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");

		// find member_name_obj from argL0
		((InstanceOop *)argL0)->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
		member_name_obj = (InstanceOop *)oop;

		member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
		// attention: if not `invokeStatic`: need to push `argL1` into `_stack`.
		int flags = ((IntOop *)oop)->value;
		int ref_kind = ((flags & 0xF000000) >> 24);
		if (ref_kind != 6) {		// not `invokeStatic`
			_stack.push_front(argL1);
		}
	} else if (_this->get_klass()->get_name() == L"java/lang/invoke/BoundMethodHandle$Species_L") {
		// 这种情况，_stack 的参数是：_stack is (_this:[Species_L], argument0:[Species_L](including DirectMethodHandle in argL0), arguments...)
		_this->get_field_value(SPECIES_L ":argL0:" OBJ, &oop);		// sample: klass:[DirectMethodHandle], toString():[MethodHandle(PrintStream,String)void]
		auto argL0 = oop;		// maybe MethodType...?
//		std::wcout << toString((InstanceOop *)_this, thread) << std::endl;		// delete
		assert(argL0->get_klass()->get_name() != L"java/lang/invoke/DirectMethodHandle");

		// get the `argument0`, in it we can get the `DirectMethodHandle`.
		InstanceOop *another = (InstanceOop *)_stack.front();	_stack.pop_front();		// 竟然还是 species_L...
		another->get_field_value(SPECIES_L ":argL0:" OBJ, &oop);
		argL0 = oop;

//		std::wcout << toString((InstanceOop *)another, thread) << std::endl;		// delete
		assert(argL0->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");

		// find member_name_obj from argL0
		((InstanceOop *)argL0)->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
		member_name_obj = (InstanceOop *)oop;

		member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
		// attention: if not `invokeStatic`: need to push `argL1` into `_stack`.
		int flags = ((IntOop *)oop)->value;
		int ref_kind = ((flags & 0xF000000) >> 24);
		if (ref_kind != 6) {		// must be `invokeStatic` because there's no `argL1`.
			assert(false);
		}
	} else if (_this->get_klass()->get_name() == L"java/lang/invoke/SimpleMethodHandle") {
		// if _this is a SimpleMethodHandle, then ignore it (I don't want to make a check).
		// then get the next argument, may be the **REAL** DirectMethodHandle.
		InstanceOop *real_method_handle = (InstanceOop *)_stack.front();	_stack.pop_front();
		assert(real_method_handle->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");

		// find member_name_obj from `DirectMethodHandle`
		((InstanceOop *)real_method_handle)->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
		member_name_obj = (InstanceOop *)oop;

		member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
		// attention: if not `invokeStatic`: need to push `argL1` into `_stack`.
		int flags = ((IntOop *)oop)->value;
		int ref_kind = ((flags & 0xF000000) >> 24);
		if (ref_kind != 6) {		// may be `invokeStatic`?
//			_stack.push_front(argL1);
			assert(false);
		}
	} else if (_this->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle") {		// 如果特别坦率，上来就是 target，那么直接取得 MemberName 即可～
		InstanceOop *real_method_handle = (InstanceOop *)_this;

		// find member_name_obj from `DirectMethodHandle`
		((InstanceOop *)real_method_handle)->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
		member_name_obj = (InstanceOop *)oop;

		member_name_obj->get_field_value(MEMBERNAME L":flags:I", &oop);
		// attention: if not `invokeStatic`: need to push `argL1` into `_stack`.
		int flags = ((IntOop *)oop)->value;
		int ref_kind = ((flags & 0xF000000) >> 24);
		if (ref_kind != 6) {		// may be `invokeStatic`?
//			_stack.push_front(argL1);
			assert(false);
		}
	} else {
		std::wcout << _this->get_klass()->get_name() << std::endl;
		assert(false);
	}

	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);
}

// 返回 fnPtr.
void *java_lang_invoke_methodHandle_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

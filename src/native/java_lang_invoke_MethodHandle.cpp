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
#include "utils/utils.hpp"

static unordered_map<wstring, void*> methods = {
    {L"invoke:([" OBJ ")" OBJ,								(void *)&JVM_Invoke},
    {L"invokeBasic:([" OBJ ")" OBJ,							(void *)&JVM_InvokeBasic},
    {L"invokeExact:([" OBJ ")" OBJ,							(void *)&JVM_InvokeExact},
};

void argument_unboxing(Method *method, list<Oop *> & args)		// Unboxing args for Integer, Double ... to int, double, [automatically] etc.
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
		} else {						// can be all except: primitive klass.
			if (oop->get_klass() == nullptr) {	// already primitive type
				temp.push_back(oop);
				continue;
			}									// else, not primitive type
			wstring klass_name = oop->get_klass()->get_name();
			InstanceOop *real_oop = (InstanceOop *)oop;
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
	if (return_type == L"V") {
		return ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Void"))->new_instance();
	}

	if (basic_type_oop == nullptr)	return nullptr;
	if (basic_type_oop->get_ooptype() != OopType::_BasicTypeOop)	return (InstanceOop *)basic_type_oop;


	Method *target_method;
	switch (((BasicTypeOop *)basic_type_oop)->get_type()) {
		case Type::BOOLEAN: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(BOOLEAN0));
			target_method = box_klass->get_this_class_method(L"valueOf:(Z)Ljava/lang/Boolean;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::BYTE: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(BYTE0));
			target_method = box_klass->get_this_class_method(L"valueOf:(B)Ljava/lang/Byte;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::SHORT: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(SHORT0));
			target_method = box_klass->get_this_class_method(L"valueOf:(S)Ljava/lang/Short;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::CHAR: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(CHARACTER0));
			target_method = box_klass->get_this_class_method(L"valueOf:(C)Ljava/lang/Character;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::INT: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(INTEGER0));
			target_method = box_klass->get_this_class_method(L"valueOf:(I)Ljava/lang/Integer;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new IntOop(((IntOop *)basic_type_oop)->value)});
		}
		case Type::FLOAT: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(FLOAT0));
			target_method = box_klass->get_this_class_method(L"valueOf:(F)Ljava/lang/Float;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new FloatOop(((FloatOop *)basic_type_oop)->value)});
		}
		case Type::LONG: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(LONG0));
			target_method = box_klass->get_this_class_method(L"valueOf:(J)Ljava/lang/Long;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new LongOop(((LongOop *)basic_type_oop)->value)});
		}
		case Type::DOUBLE: {
			auto box_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(DOUBLE0));
			target_method = box_klass->get_this_class_method(L"valueOf:(D)Ljava/lang/Double;");		// static
			assert(target_method != nullptr);
			return (InstanceOop *)thread->add_frame_and_execute(target_method, {new DoubleOop(((DoubleOop *)basic_type_oop)->value)});
		}
		default:
			assert(false);
	};
}

Oop *invoke(InstanceOop *member_name_obj, list<Oop *> & _stack, vm_thread *thread)
{
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

	auto real_klass = ((InstanceKlass *)klass);
	wstring real_name = java_lang_string::stringOop_to_wstring(name);
	if (real_name == L"<clinit>" || real_name == L"<init>") {
		assert(false);		// can't be the two names.
	}

	// 0. create a empty wstring: descriptor
	wstring descriptor = get_member_name_descriptor(real_klass, real_name, type);
	// 1. get the signature
	wstring signature = real_name + L":" + descriptor;
	// 2. get the target method
	Method *target_method = get_member_name_target_method(real_klass, signature, ref_kind);

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
		   || (_stack.size() == 1 && _stack.front()->get_klass()->get_name() == L"[Ljava/lang/Object;"));

	if (size != _stack.size()) {
		ObjArrayOop *obj_arr = (ObjArrayOop *)_stack.front();	_stack.pop_front();
		assert(obj_arr->get_length() == size);
		for (int i = 0; i < obj_arr->get_length(); i ++) {
			_stack.push_back((*obj_arr)[i]);
		}
	}

	// unboxing
	argument_unboxing(target_method, _stack);

	// 3. call it!		// return maybe: BasicTypeOop, ArrayOop, InstanceOop... all.
	Oop *result = thread->add_frame_and_execute(target_method, _stack);

	// boxing
	Oop *real_result = return_val_boxing(result, thread, target_method->return_type());

	return real_result;
}

void JVM_Invoke(list<Oop *> & _stack){

	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop [0]: _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop [length-1]: vm_thread
	_stack.pop_back();															// pop [length-2]: CallerKlassMirror *.

	// now _stack are all arguments.
	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

	// get the **REAL MemberName** from Table through methodType...
	InstanceOop *member_name_obj = find_table_if_match_methodType(methodType);

	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);
}

void JVM_InvokeBasic(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop [0]: _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop [length-1]: vm_thread
	_stack.pop_back();															// pop [length-2]: CallerKlassMirror *.

	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

	assert(_this->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle");		// DirectMethodHandle include the MemberName！！

	_this->get_field_value(DIRECTMETHODHANDLE L":member:" MN, &oop);
	InstanceOop *member_name_obj = (InstanceOop *)oop;

	// invokebasic: all arguments are in stack. no need to push a `this`.
	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);

}

void JVM_InvokeExact(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// pop [0]: _this
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();			// pop [length-1]: vm_thread
	_stack.pop_back();															// pop [length-2]: CallerKlassMirror *.

	Oop *oop;
	_this->get_field_value(METHODHANDLE L":type:Ljava/lang/invoke/MethodType;", &oop);
	InstanceOop *methodType = (InstanceOop *)oop;

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
		// _stack arguments：_stack is (_this:[Species_L], argument0:[Species_L](including DirectMethodHandle in argL0), arguments...)
		_this->get_field_value(SPECIES_L ":argL0:" OBJ, &oop);		// sample: klass:[DirectMethodHandle], toString():[MethodHandle(PrintStream,String)void]
		auto argL0 = oop;		// maybe MethodType...?
//		std::wcout << toString((InstanceOop *)_this, thread) << std::endl;		// delete
		assert(argL0->get_klass()->get_name() != L"java/lang/invoke/DirectMethodHandle");

		if (_stack.size() == 0) {
			_stack.push_back(argL0);		// L0 is the target... like ()Runnable
			return;
		}

		// get the `argument0`, in it we can get the `DirectMethodHandle`.
		InstanceOop *another = (InstanceOop *)_stack.front();	_stack.pop_front();		// another species_L...
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
	} else if (_this->get_klass()->get_name() == L"java/lang/invoke/DirectMethodHandle") {
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
//			assert(false);
		}
	} else {
		std::wcout << _this->get_klass()->get_name() << std::endl;
		assert(false);
	}

	Oop *real_result = invoke(member_name_obj, _stack, thread);

	_stack.push_back(real_result);
}

void *java_lang_invoke_methodHandle_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

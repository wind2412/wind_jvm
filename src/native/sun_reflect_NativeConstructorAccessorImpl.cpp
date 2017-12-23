/*
 * sun_reflect_NativeConstructorAccessorImpl.cpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#include "native/sun_reflect_NativeConstructorAccessorImpl.hpp"
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"newInstance0:(" CTR "[" OBJ ")" OBJ,				(void *)&JVM_NewInstanceFromConstructor},
};

void JVM_NewInstanceFromConstructor(list<Oop *> & _stack){		// static
	InstanceOop *ctor = (InstanceOop *)_stack.front();	_stack.pop_front();
	ObjArrayOop *objs = (ObjArrayOop *)_stack.front();	_stack.pop_front();
	vm_thread & thread = *(vm_thread *)_stack.back();		_stack.pop_back();

	Oop *result;
	// get target InstanceKlass
	ctor->get_field_value(CONSTRUCTOR L":clazz:Ljava/lang/Class;", &result);
	InstanceKlass *target_klass = ((InstanceKlass *)((MirrorOop *)result)->get_mirrored_who());
	// new an empty object
	Oop *init = target_klass->new_instance();
	// get shared_ptr<Method> of <init>
	ctor->get_field_value(CONSTRUCTOR L":slot:I", &result);
	shared_ptr<Method> target_method = target_klass->search_method_in_slot(((IntOop *)result)->value);
	assert(target_method->get_name() == L"<init>");
	// get arg lists types
//	assert(ctor->get_field_value(CONSTRUCTOR L":parameterTypes:[Ljava/lang/Class;", &result));
//	assert(result->get_ooptype() == OopType::_ObjArrayOop);
//	ObjArrayOop *args = (ObjArrayOop *)result;
	// check match?
//	vector<>		// TODO:
	// parse objs to list<Oop *>
	list<Oop *> arg_list;
	arg_list.push_back(init);		// push `this` first
	if (objs != nullptr)				// pitfall: if objs == nullptr, it must be <init>:()V. no args.
		for (int i = 0; i < objs->get_length(); i ++) {
			arg_list.push_back((*objs)[i]);
		}
	// launch and construct, no return val.
	thread.add_frame_and_execute(target_method, arg_list);
	// save.
	_stack.push_back(init);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) use Constructor:[<init>:" << target_method->get_descriptor() << "] to initialize a new obj of type [" << target_klass->get_name() << "]." << std::endl;
#endif
}



// 返回 fnPtr.
void *sun_reflect_nativeConstructorAccessorImpl_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

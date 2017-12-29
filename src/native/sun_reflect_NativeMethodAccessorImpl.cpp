/*
 * sun_reflect_NativeMethodAccessorImpl.cpp
 *
 *  Created on: 2017年12月17日
 *      Author: zhengxiaolin
 */

#include "native/sun_reflect_NativeMethodAccessorImpl.hpp"
#include "native/java_lang_invoke_MethodHandle.hpp"
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"invoke0:(" MHD OBJ "[" OBJ ")" OBJ,				(void *)&JVM_Invoke0},
};

void JVM_Invoke0(list<Oop *> & _stack){		// static
	InstanceOop *method = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();	// maybe null. `_this` is for `method` variable, not for this `Invoke0` method.
	ObjArrayOop *objs = (ObjArrayOop *)_stack.front();	_stack.pop_front();
	vm_thread *thread = (vm_thread *)_stack.back();		_stack.pop_back();

	Oop *result;

	// get target InstanceKlass
	method->get_field_value(METHOD L":clazz:Ljava/lang/Class;", &result);
	InstanceKlass *target_klass = ((InstanceKlass *)((MirrorOop *)result)->get_mirrored_who());
	// get the real method:
	method->get_field_value(METHOD L":slot:I", &result);
	Method *target_method = target_klass->search_method_in_slot(((IntOop *)result)->value);

	// get the real arg:
	list<Oop *> arg;
	if (!target_method->is_static()) {
		arg.push_back(_this);
	}
	for (int i = 0; i < objs->get_length(); i ++) {
		arg.push_back((*objs)[i]);
	}

	// unboxing
	argument_unboxing(target_method, arg);

	// check
	int size = target_method->parse_argument_list().size();
	if (!target_method->is_static()) {
		size ++;
	}
	assert(arg.size() == size);

	// call
	result = thread->add_frame_and_execute(target_method, arg);

	// boxing
	Oop *real_result = return_val_boxing(result, thread, target_method->return_type());

	_stack.push_back(real_result);
}



void *sun_reflect_nativeMethodAccessorImpl_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



/*
 * java_lang_reflect_Array.cpp
 *
 *  Created on: 2017年12月9日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_reflect_Array.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/os.hpp"
#include "classloader.hpp"

static unordered_map<wstring, void*> methods = {
    {L"newArray:(" CLS "I)" OBJ,							(void *)&JVM_NewArray},
};

void JVM_NewArray(list<Oop *> & _stack){		// static
	MirrorOop *mirror = (MirrorOop *)_stack.front();	_stack.pop_front();
	int length = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	assert(length >= 0);
	auto real_klass = mirror->get_mirrored_who();

	if (real_klass == nullptr) {	// 1. should create a primitive-type array.
		wstring arr_name = L"[" + mirror->get_extra();
		assert(arr_name != L"[V");		// not void
		auto arr_klass = std::static_pointer_cast<TypeArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(arr_name));
		assert(arr_klass != nullptr);
		_stack.push_back(arr_klass->new_instance(length));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new an primitive type array." << std::endl;
#endif
		return;
	} else if (real_klass->get_type() == ClassType::InstanceClass) {
		auto real_instance_klass = std::static_pointer_cast<InstanceKlass>(real_klass);
		ClassLoader *loader = real_instance_klass->get_classloader();
		shared_ptr<ObjArrayKlass> arr_klass;
		if (loader == nullptr) {
			arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[L" + real_instance_klass->get_name() + L";"));
		} else {
			arr_klass = std::static_pointer_cast<ObjArrayKlass>(loader->loadClass(L"[L" + real_instance_klass->get_name() + L";"));
		}
		assert(arr_klass != nullptr);
		_stack.push_back(arr_klass->new_instance(length));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new an InstanceKlass array." << std::endl;
#endif
		return;
	} else if (real_klass->get_type() == ClassType::TypeArrayClass) {
		auto arr_klass = std::static_pointer_cast<TypeArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[" + real_klass->get_name()));	// add one dimension
		assert(arr_klass != nullptr);
		_stack.push_back(arr_klass->new_instance(length));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new a TypeArrayKlass's array." << std::endl;
#endif
		return;
	} else if (real_klass->get_type() == ClassType::ObjArrayClass) {
		auto inner_element_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass)->get_element_klass();		// return InstanceKlass
		ClassLoader *loader = inner_element_klass->get_classloader();
		shared_ptr<ObjArrayKlass> arr_klass;
		if (loader == nullptr) {
			arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[" + inner_element_klass->get_name()));
		} else {
			arr_klass = std::static_pointer_cast<ObjArrayKlass>(loader->loadClass(L"[" + inner_element_klass->get_name()));
		}
		assert(arr_klass != nullptr);
		_stack.push_back(arr_klass->new_instance(length));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new a ObjectArrayKlass's array." << std::endl;
#endif
		return;
	} else {
		std::wcout << real_klass->get_type() << std::endl;
		assert(false);
	}

}



// 返回 fnPtr.
void *java_lang_reflect_array_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

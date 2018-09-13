/*
 * java_security_AccessController.cpp
 *
 *  Created on: 2017年11月25日
 *      Author: zhengxiaolin
 */

#include "native/java_security_AccessController.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "classloader.hpp"

static unordered_map<wstring, void*> methods = {
    {L"doPrivileged:(" PA ")" OBJ,				(void *)&JVM_DoPrivileged},
    {L"doPrivileged:(" PA ACC ")" OBJ,			(void *)&JVM_DoPrivileged},
    {L"doPrivileged:(" PEA ")" OBJ,				(void *)&JVM_DoPrivileged},
    {L"doPrivileged:(" PEA ACC ")" OBJ,			(void *)&JVM_DoPrivileged},
    {L"getStackAccessControlContext:()" ACC,		(void *)&JVM_GetStackAccessControlContext},
};

// this method, I simplified it to just run the `run()` method.
void JVM_DoPrivileged (list<Oop*>& _stack)
{
	// static
	Oop* pa = (Oop*) (_stack.front ());
	_stack.pop_front ();
	vm_thread & thread = (*(vm_thread*) (_stack.back ()));
	_stack.pop_back ();
	assert(pa != nullptr);
	// try:
	/**
	 *  interface A<T> {		// simple: AccessController.doPrivileged()。
			T run();
		}

		class Generic<T> {

			void run(A a) {

			}

			public static void main(String[] args) {
				Generic<String> u = new Generic<>();
				u.run(new A<String>(){
					public String run() {
						return new String();
					}
				});
			}

		编译运行之后会发现 Generic$1.class 内部类中，含有两个同样签名为 `run()` 的方法。
		而 `run() OBJ` 自动转调用了 `run() STR`。
	 */
	Method *method = ((InstanceKlass *)pa->get_klass())->get_this_class_method(L"run:()" VOD);
	if (method == nullptr) {
//		method = ((InstanceKlass *)pa->get_klass())->get_this_class_method(L"run:()" STR);
//		if (method == nullptr) {
//			method = ((InstanceKlass *)pa->get_klass())->get_this_class_method(L"run:()" FLD);
//			if (method == nullptr) {
//				method = ((InstanceKlass *)pa->get_klass())->get_this_class_method(L"run:()Lsun/reflect/ReflectionFactory;");
//			}
//		}
		method = ((InstanceKlass *)pa->get_klass())->get_this_class_method(L"run:()" OBJ);
	}
	assert(method != nullptr);

	Oop* result = thread.add_frame_and_execute (method, { pa }); // load the `this` obj

	bool substitute = false;
	if (result != nullptr && result->get_ooptype() != OopType::_BasicTypeOop && result->get_klass()->get_type() == ClassType::InstanceClass) {	// same as `(Bytecode)invokeVirtual` 's exception judge.
		auto klass = ((InstanceKlass *)result->get_klass());
		auto throwable_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Throwable"));
		if (klass == throwable_klass || klass->check_parent(throwable_klass)) {
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) find the last frame's exception: [" << klass->get_name() << "]. will goto exception_handler!" << std::endl;
#endif
			substitute = true;
		}
	}

	if (substitute) {
		_stack.push_back (pa);
	} else {
		_stack.push_back (result);
	}
}

// I don't get a snapshot... return nullptr.
void JVM_GetStackAccessControlContext(list<Oop *> & _stack){	// static
	_stack.push_back(nullptr);
}



void *java_security_accesscontroller_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}




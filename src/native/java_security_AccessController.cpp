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
	// [x]不支持泛型的代价。以后补上。---- [√] 看来不需要支持了。注释掉的部分是以前的。因为后来发现，
	// java 字节码中，由于 java 是伪泛型，因此编译器会除了用户指定泛型的 `run:() STR` 等之外，还产生一个原本的 `run:() OBJ`。虚拟机由于不支持泛型，会调用 SYNTHETIC 的 `run:() OBJ`。
	// 而 `run:() OBJ` 会转调用特化的 `run:() STR`。而由于是编译器生成的，因此虽然函数签名完全一致，却并不造成重载。
	// 可以试试：
	/**
	 *  interface A<T> {		// 其实是一个简化版的 AccessController.doPrivileged()。
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
	shared_ptr<Method> method = std::static_pointer_cast<InstanceKlass>(pa->get_klass())->get_this_class_method(L"run:()" VOD);
	if (method == nullptr) {
//		method = std::static_pointer_cast<InstanceKlass>(pa->get_klass())->get_this_class_method(L"run:()" STR);
//		if (method == nullptr) {
//			method = std::static_pointer_cast<InstanceKlass>(pa->get_klass())->get_this_class_method(L"run:()" FLD);
//			if (method == nullptr) {
//				method = std::static_pointer_cast<InstanceKlass>(pa->get_klass())->get_this_class_method(L"run:()Lsun/reflect/ReflectionFactory;");
//			}
//		}
		method = std::static_pointer_cast<InstanceKlass>(pa->get_klass())->get_this_class_method(L"run:()" OBJ);
	}
	assert(method != nullptr);

	Oop* result = thread.add_frame_and_execute (method, { pa }); // load the `this` obj

	// **IMPORTANT** judge whether returns an Exception!!!		// TODO: bug：如果在 run() 中 throw 的话......像是 openjdk: URLClassLoader::findClass() 一样......
																// 应当捕获所有 Exception，并且变成 PrivilegeException 抛出！！
	bool substitute = false;
	if (result != nullptr && result->get_ooptype() != OopType::_BasicTypeOop && result->get_klass()->get_type() == ClassType::InstanceClass) {	// same as `(Bytecode)invokeVirtual` 's exception judge.
		auto klass = std::static_pointer_cast<InstanceKlass>(result->get_klass());
		auto throwable_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Throwable"));
		if (klass == throwable_klass || klass->check_parent(throwable_klass)) {
#ifdef DEBUG
std::wcout << "(DEBUG) find the last frame's exception: [" << klass->get_name() << "]. will goto exception_handler!" << std::endl;
#endif
			substitute = true;
		}
	}

	if (substitute) {
		_stack.push_back (pa);			// 使用 PriviledgeException 代替返回的 Exception 返回！！
	} else {
		_stack.push_back (result);
	}
}

// I don't get a snapshot... return nullptr.
void JVM_GetStackAccessControlContext(list<Oop *> & _stack){	// static
	_stack.push_back(nullptr);
}



// 返回 fnPtr.
void *java_security_accesscontroller_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}




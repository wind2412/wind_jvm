/*
 * main.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */
#include "wind_jvm.hpp"
#include "runtime/klass.hpp"
#include "runtime/java_lang_class.hpp"
#include "runtime/java_lang_string.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"
#include <boost/regex.hpp>

struct temp {		// pthread aux struct...
	wind_jvm *jvm;
	const vector<wstring> *arg;
};

auto scapegoat = [](void *pp) -> void *{
	temp *real = (temp *)pp;
	real->jvm->start(*real->arg);
	return nullptr;
};

wind_jvm::wind_jvm(const wstring & main_class_name, const vector<wstring> & argv) : main_class_name(boost::regex_replace(main_class_name, boost::wregex(L"\\."), L"/")), rsp(-1), pc(0)
{
	// start one thread
	temp p;
	p.jvm = this;
	p.arg = &argv;

//	auto scapegoat = [](void *pp) -> void *{
//		temp *real = (temp *)pp;
//	std::cout << real->jvm << "...???" << std::endl;		// delete
//		real->jvm->start(*real->arg);				// TODO: 原来如此......这个东西是被析构掉了。。。。。所以产生了未定义行为...... 由于当时在这里定义的，但是不知道为何，最后的 real->jvm，即 this 会经常变成 0x0, 0x4...但是还没有搞清楚为什么。
//		return nullptr;
//	};

	pthread_create(&tid, nullptr, scapegoat, &p);
}

void wind_jvm::start(const vector<wstring> & argv)
{
	// detach itself.
	pthread_detach(pthread_self());

	assert(main_class_name != L"");
	{
		java_lang_class::init();		// must init !!!
		BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class");
		java_lang_class::fixup_mirrors();	// only [basic types] + java.lang.Class + java.lang.Object
	}
	shared_ptr<Klass> main_class = MyClassLoader::get_loader().loadClass(main_class_name);		// this time, "java.lang.Object" has been convert to "java/lang/Object".
	shared_ptr<Method> main_method = std::static_pointer_cast<InstanceKlass>(main_class)->get_static_void_main();
	assert(main_method != nullptr);
	// TODO: 方法区，多线程，堆区，垃圾回收！现在的目标只是 BytecodeExecuteEngine，将来要都加上！！
	// TODO: 别忘了强制执行 main_class 的 clinit 方法！！
	// TODO：需要把 argv 这个 C++ 对象强行提升成为 Java 的 String[]'s Oop！

	// first execute <clinit> if has
	BytecodeEngine::initial_clinit(std::static_pointer_cast<InstanceKlass>(main_class), *this);
	// second execute [public static void main].

	// new a String[].
	ObjArrayOop *string_arr_oop = (ObjArrayOop *)std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/String;"))->new_instance(argv.size());
	auto iter = system_classmap.find(L"java/lang/String.class");
	assert(iter != system_classmap.end());
	auto string_klass = std::static_pointer_cast<InstanceKlass>((*iter).second);
	for (int i = 0; i < argv.size(); i ++) {
		(*string_arr_oop)[i] = java_lang_string::intern(argv[i]);
	}
	this->vm_stack.push_back(StackFrame(main_method, nullptr, nullptr, {string_arr_oop}));		// TODO: 暂时设置 main 方法的 return_pc 和 prev 全是 nullptr。
	this->execute();
}

void wind_jvm::execute()
{
	while(!vm_stack.empty()) {		// run over when stack is empty...
		StackFrame & cur_frame = vm_stack.back();
		if (cur_frame.method->is_native()) {
			pc = nullptr;
			// TODO: native.
			std::cerr << "Doesn't support native now." << std::endl;
			assert(false);
		} else {
			auto code = cur_frame.method->get_code();
			// TODO: support Code attributes......
			if (code->code_length == 0) {
				std::cerr << "empty method??" << std::endl;
				assert(false);		// for test. Is empty method valid ??? I dont know...
			}
			pc = code->code;
			Oop * return_val = BytecodeEngine::execute(*this, vm_stack.back());
			if (cur_frame.method->is_void()) {		// TODO: in fact, this can be delete. Because It is of no use.
				assert(return_val == nullptr);
				// do nothing
			} else {
				cur_frame.op_stack.push(return_val);
			}
		}
		vm_stack.pop_back();	// another half push_back() is in wind_jvm() constructor.
	}
}

Oop * wind_jvm::add_frame_and_execute(shared_ptr<Method> new_method, const std::list<Oop *> & list) {
	this->vm_stack.push_back(StackFrame(new_method, nullptr, nullptr, list));
	Oop * result = BytecodeEngine::execute(*this, this->vm_stack.back());
	this->vm_stack.pop_back();
	return result;
}

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
#include <boost/regex.hpp>

wind_jvm::wind_jvm(const wstring & main_class_name, const vector<wstring> & argv) : main_class_name(boost::regex_replace(main_class_name, boost::wregex(L"\\."), L"/")), rsp(-1), pc(0)
{
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


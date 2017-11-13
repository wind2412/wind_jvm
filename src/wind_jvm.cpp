/*
 * main.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "wind_jvm.hpp"
#include "runtime/klass.hpp"
#include <boost/regex.hpp>

wind_jvm::wind_jvm(const wstring & main_class_name, const vector<wstring> & argv) : main_class_name(boost::regex_replace(main_class_name, boost::wregex(L"\\."), L"/")), rsp(-1), pc(0) {
	shared_ptr<Klass> main_class = MyClassLoader::get_loader().loadClass(main_class_name);		// this time, "java.lang.Object" has been convert to "java/lang/Object".
	shared_ptr<Method> main_method = std::static_pointer_cast<InstanceKlass>(main_class)->get_static_void_main();
	assert(main_method != nullptr);
	// TODO: 方法区，多线程，堆区，垃圾回收！现在的目标只是 BytecodeExecuteEngine，将来要都加上！！
	// TODO: 别忘了强制执行 main_class 的 clinit 方法！！
	// TODO：需要把 argv 这个 C++ 对象强行提升成为 Java 的 String[]'s Oop！

	// first execute <clinit> if has
	BytecodeEngine::initial_clinit(std::static_pointer_cast<InstanceKlass>(main_class), *this);
	// second execute [public static void main].
	ObjArrayOop *stringoop = new ObjArrayOop(std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/String;")), argv.size());			// 手动调用
	this->vm_stack.push_back(StackFrame(nullptr, main_method, nullptr, nullptr, {(uint64_t)stringoop}));		// TODO: 暂时设置 main 方法的 return_pc 和 prev 全是 nullptr。
	this->execute();
}


/*
 * main.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */
#include "wind_jvm.hpp"
#include "runtime/klass.hpp"
#include "native/java_lang_Class.hpp"
#include "native/java_lang_String.hpp"
#include "native/java_lang_Thread.hpp"
#include "native/native.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"
#include "runtime/thread.hpp"
#include <boost/regex.hpp>

auto scapegoat = [](void *pp) -> void *{
	temp *real = (temp *)pp;
	real->jvm->start(*real->arg);
	return nullptr;
};

wind_jvm::wind_jvm(const wstring & main_class_name, const vector<wstring> & argv) : main_class_name(boost::regex_replace(main_class_name, boost::wregex(L"\\."), L"/")), rsp(-1), pc(0)
{
	// start one thread
//	temp p;				// p 变成局部变量之后会引发大 bug ？？！！卧槽 ？？？！！		// 重点！！ 看来是要变成全局变量才行... ！！很可能是析构掉了？？
	p.jvm = this;
	p.arg = &argv;

//	auto scapegoat = [](void *pp) -> void *{
//		temp *real = (temp *)pp;
//	std::cout << real->jvm << "...???" << std::endl;		// delete
//		real->jvm->start(*real->arg);				// TODO: 原来如此......这个东西是被析构掉了。。。。。所以产生了未定义行为...... 由于当时在这里定义的，但是不知道为何，最后的 real->jvm，即 this 会经常变成 0x0, 0x4...但是还没有搞清楚为什么。
//		return nullptr;
//	};

	// 在这里，需要初始化全局变量。线程还没有开启。
	// TODO: 各种 system_map 这类，以及全局的 BootStrapLoader 这类，线程没有加锁，全可以直接访问，多线程不安全。别忘了加！！
	init_native();

	pthread_t tid;
	pthread_create(&tid, nullptr, scapegoat, &p);
}

void wind_jvm::start(const vector<wstring> & argv)
{
	// detach itself.
	pthread_detach(pthread_self());

	// TODO: 这里在多线程下会发生什么？？？

	// init.
	{
		java_lang_class::init();		// must init !!!
		BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Class");
		java_lang_class::fixup_mirrors();	// only [basic types] + java.lang.Class + java.lang.Object

		// load String.class
		auto string_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/String"));

		// 1. create a [half-completed] Thread obj, using the ThreadGroup obj.(for currentThread(), this must be create first!!)
		auto thread_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Thread"));
		// TODO: 要不要放到全局？
		InstanceOop *init_thread = thread_klass->new_instance();
		BytecodeEngine::initial_clinit(thread_klass, *this);		// first <clinit>!
		// inject!!
		init_thread->set_field_value(L"eetop:J", new LongOop((uint64_t)pthread_self()));		// TODO: 这样没有移植性！！要改啊！！！虽然很方便......其实在 linux 下，也是 8 bytes......
		init_thread->set_field_value(L"priority:I", new IntOop(NormPriority));	// TODO: ......		// runtime/thread.cpp:1026
		// add this Thread obj to ThreadTable!!!	// ......在这里放入的 init_thread 并没有初始化完全。因为它还没有执行构造函数。不过，那也必须放到表中了。因为在 <init> 执行的时候，内部有其他的类要调用 currentThread...... 所以不放入表中不行啊......
		ThreadTable::add_a_thread(pthread_self(), init_thread);

		// 2. create a [System] ThreadGroup obj.
		auto threadgroup_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/ThreadGroup"));
		// TODO: 要不要放到全局？
		InstanceOop *init_threadgroup = threadgroup_klass->new_instance();
		BytecodeEngine::initial_clinit(threadgroup_klass, *this);		// first <clinit>!
		{	// 注意：这里创建了全局的第一个 System ThreadGroup !!
			// TODO: 放到全局！
			std::list<Oop *> list;
			list.push_back(init_threadgroup);	// $0 = this
			// execute method: java/lang/ThreadGroup.<init>:()V --> private Method!!
			shared_ptr<Method> target_method = threadgroup_klass->get_this_class_method(L"<init>:()V");
			assert(target_method != nullptr);
			this->add_frame_and_execute(target_method, list);
		}

		// 3. create a [Main] ThreadGroup obj.
		InstanceOop *main_threadgroup = threadgroup_klass->new_instance();
		{
			// inject it into `init_thread`!! 否则，届时在 java/lang/SecurityManager <clinit> 时，会自动 getCurrentThread --> get 到 main_threadgroup --> get 到 system_threadgroup. 所以必须先行注入。
			// hack...
			init_thread->set_field_value(L"group:Ljava/lang/ThreadGroup;", main_threadgroup);
		}
		assert(this->vm_stack.size() == 0);
		{	// 注意：这里创建了针对此 main 的第二个 System ThreadGroup !!用第一个 System ThreadGroup 作为参数！
			// TODO: pthread_mutex!!
			std::list<Oop *> list;
			list.push_back(main_threadgroup);	// $0 = this
			list.push_back(nullptr);				// $1 = nullptr
			list.push_back(init_threadgroup);	// $2 = init_threadgroup
			list.push_back(java_lang_string::intern(L"main"));	// $3 = L"main"
			// execute method: java/lang/ThreadGroup.<init>:()V --> private Method!!		// 直接调用私有方法！为了避过狗日的 java/lang/SecurityManager 的检查......我也是挺拼的......QAQ
			// TODO: 因为这里是直接调用了私方法，所以有可能是不可移植的。因为它私方法有可能变。
			shared_ptr<Method> target_method = threadgroup_klass->get_this_class_method(L"<init>:(Ljava/lang/Void;Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
			assert(target_method != nullptr);
			this->add_frame_and_execute(target_method, list);
		}

		// 4. [complete] the Thread obj using the [uncomplete] main_threadgroup.
		{
			std::list<Oop *> list;
			list.push_back(init_thread);		// $0 = this
			list.push_back(main_threadgroup);	// $1 = [main_threadGroup]
			list.push_back(java_lang_string::intern(L"main"));	// $2 = L"main"
			// execute method: java/lang/Thread.<init>:(ThreadGroup, String)V --> public Method.
			shared_ptr<Method> target_method = thread_klass->get_this_class_method(L"<init>:(Ljava/lang/ThreadGroup;Ljava/lang/String;)V");
			assert(target_method != nullptr);
			this->add_frame_and_execute(target_method, list);
		}

	}

	// for test
	assert(false);		// 这里开始才是高能......都不知道能不能成功...... 而且 exec 还有没被加载......
	auto klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"sun/misc/Launcher$AppClassLoader"));

	// TODO: 不应该用 MyClassLoader ！！ 应该用 Java 写的 AppClassLoader!!!
	shared_ptr<Klass> main_class = MyClassLoader::get_loader().loadClass(main_class_name);		// this time, "java.lang.Object" has been convert to "java/lang/Object".
	shared_ptr<Method> main_method = std::static_pointer_cast<InstanceKlass>(main_class)->get_static_void_main();
	assert(main_method != nullptr);
	// TODO: 方法区，多线程，堆区，垃圾回收！现在的目标只是 BytecodeExecuteEngine，将来要都加上！！

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

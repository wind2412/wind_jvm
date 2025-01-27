/*
 * java_lang_Thread.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Thread.hpp"
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "runtime/thread.hpp"
#include "wind_jvm.hpp"
#include "utils/synchronize_wcout.hpp"

/*===-------------- from hotspot -------------------*/
static unordered_map<wstring, void*> methods = {
    {L"start0:()V",						(void *)&JVM_StartThread},
    {L"stop0:(" OBJ L")V",				(void *)&JVM_StopThread},
    {L"isAlive:()Z",						(void *)&JVM_IsThreadAlive},
    {L"suspend0:()V",					(void *)&JVM_SuspendThread},
    {L"resume0:()V",						(void *)&JVM_ResumeThread},
    {L"setPriority0:(I)V",				(void *)&JVM_SetThreadPriority},
    {L"yield:()V",						(void *)&JVM_Yield},
    {L"sleep:(J)V",						(void *)&JVM_Sleep},
    {L"currentThread:()" THD,			(void *)&JVM_CurrentThread},
    {L"countStackFrames:()I",			(void *)&JVM_CountStackFrames},
    {L"interrupt0:()V",					(void *)&JVM_Interrupt},
    {L"isInterrupted:(Z)Z",				(void *)&JVM_IsInterrupted},
    {L"holdsLock:(" OBJ L")Z",			(void *)&JVM_HoldsLock},
    {L"getThreads:()[" THD,				(void *)&JVM_GetAllThreads},
    {L"dumpThreads:([" THD L")[[" STE,	(void *)&JVM_DumpThreads},
    {L"setNativeName:(" STR L")V",		(void *)&JVM_SetNativeThreadName},
};

void JVM_StartThread(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	Oop *result;
	_this->get_field_value(THREAD L":eetop:J", &result);
	LongOop *tid = (LongOop *)result;
	assert(tid->value == 0);			// must be 0. if not 0, it must be started already.

	// if class is ReferenceHandler, I won't open this thread like openjdk. it is of no use.
	if (_this->get_klass()->get_name() == L"java/lang/ref/Reference$ReferenceHandler") {
		return;
	}

	// first, find the `run()` method in `this`.
	Method *run = ((InstanceKlass *)_this->get_klass())->get_this_class_method(L"run:()V");
	assert(run != nullptr && !run->is_abstract());
	// set and start a thread.
	vm_thread *new_thread;
	wind_jvm::lock().lock();
	{
		wind_jvm::threads().push_back(vm_thread(run, {_this}));		// 1. create a thread obj.
		new_thread = &wind_jvm::threads().back();
	}
	wind_jvm::lock().unlock();

	wind_jvm::num_lock().lock();
	{
		wind_jvm::thread_num() ++;
	}
	wind_jvm::num_lock().unlock();

	// start!
	new_thread->launch(_this);

	while(true) {
		pthread_mutex_lock(&_all_thread_wait_mutex);
		if (new_thread->is_waited_for_child()) {		// wait until the new_thread wait.
			break;
		}
		pthread_mutex_unlock(&_all_thread_wait_mutex);
	}

	pthread_mutex_unlock(&_all_thread_wait_mutex);

}

void JVM_StopThread(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	Oop *obj = (Oop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsThreadAlive(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
#ifdef DEBUG
	sync_wcout{} << "the java.lang.Thread obj's address: [" << _this << "]." << std::endl;
#endif
	Oop *result;
	_this->get_field_value(THREAD L":eetop:J", &result);
	LongOop *tid = (LongOop *)result;
//	assert(tid->value != 0);	// simple check
	/**
	 * bug report: When creating object of java/lang/ref/Reference.ReferenceHandler( extends java.lang.Thread ), it will test using `setDaemon`.
	 * but this time, `eetop` is not settled, will be 0. As we see it will check isAlive ---- if it's alive, will throw Exception.
	 * so we should deprecate the `assert(tid->value != 0)` before, and when check it is 0, return false instead.
	 * public final void setDaemon(boolean on) {		// this is Thread.setDaemon() function. we can see, `isAlive()` called.
        checkAccess();
        if (isAlive()) {
            throw new IllegalThreadStateException();
        }
        daemon = on;
	   }
	 */
	if (tid->value == 0) {
#ifdef DEBUG
	sync_wcout{} << "This Thread is an empty obj, only alloc memory but not settled pthread_t. It didn't call the `start0` method! " << std::endl;
#endif
		_stack.push_back(new IntOop(0));
		return;
	}

	int ret = pthread_kill((pthread_t)tid, 0);
	if (ret == 0) {
		_stack.push_back(new IntOop(1));
#ifdef DEBUG
	sync_wcout{} << "Thread pthread_t: [" << tid->value << "] is alive! " << std::endl;
#endif
	} else if (ret == ESRCH) {
		_stack.push_back(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "Thread pthread_t: [" << tid->value << "] is dead... " << std::endl;
#endif
	} else {
		// EINVAL
		assert(false);
	}
}
void JVM_SuspendThread(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_ResumeThread(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetThreadPriority(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	IntOop *i = (IntOop *)_stack.front();	_stack.pop_front();
	// TODO: finish it...
}
void JVM_Yield(list<Oop *> & _stack){			// static
	if (sched_yield() == -1) {
		assert(false);
	}
}
void JVM_Sleep(list<Oop *> & _stack){			// static
	LongOop *l1 = (LongOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_CurrentThread(list<Oop *> & _stack){		// static
	InstanceOop *thread_oop;
	assert(ThreadTable::detect_thread_death(pthread_self()) == false);
	thread_oop = ThreadTable::get_a_thread(pthread_self());
	assert(thread_oop != nullptr);
	_stack.push_back(thread_oop);
}
void JVM_CountStackFrames(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_Interrupt(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsInterrupted(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	bool _clear_interrupted = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	// didn't implement

	_stack.push_back(new IntOop(false));
}
void JVM_HoldsLock(list<Oop *> & _stack){		// static, no this...
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetAllThreads(list<Oop *> & _stack){	// static
	assert(false);
}
void JVM_DumpThreads(list<Oop *> & _stack){	// static
	ObjArrayOop *threads = (ObjArrayOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetNativeThreadName(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

void *java_lang_thread_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

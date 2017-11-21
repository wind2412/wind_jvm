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

void JVM_StartThread(list<Oop *> & _stack){		// static
	assert(false);
}
void JVM_StopThread(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	Oop *obj = (Oop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsThreadAlive(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
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
	assert(false);
}
void JVM_Yield(list<Oop *> & _stack){			// static
	assert(false);
}
void JVM_Sleep(list<Oop *> & _stack){			// static
	LongOop *l1 = (LongOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_CurrentThread(list<Oop *> & _stack){		// static
	InstanceOop *thread_oop;
	ThreadTable::print_table();
	assert(ThreadTable::detect_thread_death(pthread_self()) == false);
	assert((thread_oop = ThreadTable::get_a_thread(pthread_self())) != nullptr);						// TODO: 我自己都不知道这实现是否正确......多线程太诡异了......
	_stack.push_back(thread_oop);		// 返回值被压入 _stack.
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
	BooleanOop *b1 = (BooleanOop *)_stack.front();	_stack.pop_front();
	assert(false);
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

// 返回 fnPtr.
void *java_lang_thread_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

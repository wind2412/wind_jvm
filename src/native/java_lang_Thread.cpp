/*
 * java_lang_Thread.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/jni.hpp"
#include "native/java_lang_Thread.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

using std::vector;

/*===-------------- from hotspot -------------------*/
enum ThreadPriority {        // JLS 20.20.1-3
  NoPriority       = -1,     // Initial non-priority value
  MinPriority      =  1,     // Minimum priority
  NormPriority     =  5,     // Normal (non-daemon) priority
  NearMaxPriority  =  9,     // High priority, used for VMThread
  MaxPriority      = 10,     // Highest priority, used for WatcherThread
                             // ensures that VMThread doesn't starve profiler
  CriticalPriority = 11      // Critical thread priority
};

static vector<JNINativeMethod> methods = {
	{"registerNatives",	"()V",		   (void *)&Java_java_lang_thread_registerNative},
    {"start0",           "()V",        (void *)&JVM_StartThread},
    {"stop0",            "(" OBJ ")V", (void *)&JVM_StopThread},
    {"isAlive",          "()Z",        (void *)&JVM_IsThreadAlive},
    {"suspend0",         "()V",        (void *)&JVM_SuspendThread},
    {"resume0",          "()V",        (void *)&JVM_ResumeThread},
    {"setPriority0",     "(I)V",       (void *)&JVM_SetThreadPriority},
    {"yield",            "()V",        (void *)&JVM_Yield},
    {"sleep",            "(J)V",       (void *)&JVM_Sleep},
    {"currentThread",    "()" THD,     (void *)&JVM_CurrentThread},
    {"countStackFrames", "()I",        (void *)&JVM_CountStackFrames},
    {"interrupt0",       "()V",        (void *)&JVM_Interrupt},
    {"isInterrupted",    "(Z)Z",       (void *)&JVM_IsInterrupted},
    {"holdsLock",        "(" OBJ ")Z", (void *)&JVM_HoldsLock},
    {"getThreads",        "()[" THD,   (void *)&JVM_GetAllThreads},
    {"dumpThreads",      "([" THD ")[[" STE, (void *)&JVM_DumpThreads},
    {"setNativeName",    "(" STR ")V", (void *)&JVM_SetNativeThreadName},
};

// register native, I don't want to use a dylib. Origin name will be Java_java_lang_thread_registerNative.
void Java_java_lang_thread_registerNative(stack<Oop *> & _stack)
{
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}

void JVM_StartThread(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_StopThread(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	Oop *obj = (Oop *)_stack.top();	_stack.pop();
	assert(false);
}
bool JVM_IsThreadAlive(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SuspendThread(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_ResumeThread(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SetThreadPriority(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	IntOop *i = (IntOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_Yield(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_Sleep(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	LongOop *l1 = (LongOop *)_stack.top();	_stack.pop();
	assert(false);
}
InstanceOop *JVM_CurrentThread(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
IntOop *JVM_CountStackFrames(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_Interrupt(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
bool JVM_IsInterrupted(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	BooleanOop *b1 = (BooleanOop *)_stack.top();	_stack.pop();
	assert(false);
}
bool JVM_HoldsLock(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *obj = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
ObjArrayOop *JVM_GetAllThreads(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}
ObjArrayOop *JVM_DumpThreads(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	ObjArrayOop *threads = (ObjArrayOop *)_stack.top();	_stack.pop();
	assert(false);
}
void JVM_SetNativeThreadName(stack<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.top();	_stack.pop();
	InstanceOop *str = (InstanceOop *)_stack.top();	_stack.pop();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_thread_search_method(const string & str)
{
	auto iter = std::find_if(methods.begin(), methods.end(), [&str](JNINativeMethod & m){ return m.name == str; });
	if (iter != methods.end()) {
		return (*iter).fnPtr;
	}
	return nullptr;
}

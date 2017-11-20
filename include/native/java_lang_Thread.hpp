/*
 * java_lang_Thread.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_

#include "runtime/oop.hpp"
#include <stack>

using std::stack;
using std::string;

void Java_java_lang_thread_registerNative(stack<Oop *> & _stack);

void JVM_StartThread(stack<Oop *> & _stack);
void JVM_StopThread(stack<Oop *> & _stack);
bool JVM_IsThreadAlive(stack<Oop *> & _stack);
void JVM_SuspendThread(stack<Oop *> & _stack);
void JVM_ResumeThread(stack<Oop *> & _stack);
void JVM_SetThreadPriority(stack<Oop *> & _stack);
void JVM_Yield(stack<Oop *> & _stack);
void JVM_Sleep(stack<Oop *> & _stack);
InstanceOop *JVM_CurrentThread(stack<Oop *> & _stack);
IntOop *JVM_CountStackFrames(stack<Oop *> & _stack);
void JVM_Interrupt(stack<Oop *> & _stack);
bool JVM_IsInterrupted(stack<Oop *> & _stack);
bool JVM_HoldsLock(stack<Oop *> & _stack);
ObjArrayOop *JVM_GetAllThreads(stack<Oop *> & _stack);
ObjArrayOop *JVM_DumpThreads(stack<Oop *> & _stack);
void JVM_SetNativeThreadName(stack<Oop *> & _stack);

void *java_lang_thread_search_method(const string & str);

#endif /* INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_ */

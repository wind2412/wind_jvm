/*
 * java_lang_Thread.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_

#include "runtime/oop.hpp"

void Java_java_lang_thread_registerNative(InstanceOop *_this);

void JVM_StartThread(InstanceOop *_this);
void JVM_StopThread(InstanceOop *_this, Oop *obj);
bool JVM_IsThreadAlive(InstanceOop *_this);
void JVM_SuspendThread(InstanceOop *_this);
void JVM_ResumeThread(InstanceOop *_this);
void JVM_SetThreadPriority(InstanceOop *_this, IntOop *);
void JVM_Yield(InstanceOop *_this);
void JVM_Sleep(InstanceOop *_this, LongOop *);
InstanceOop *JVM_CurrentThread(InstanceOop *_this);
IntOop *JVM_CountStackFrames(InstanceOop *_this);
void JVM_Interrupt(InstanceOop *_this);
bool JVM_IsInterrupted(InstanceOop *_this, BooleanOop *);
bool JVM_HoldsLock(InstanceOop *_this, InstanceOop *obj);
ObjArrayOop *JVM_GetAllThreads(InstanceOop *_this);
ObjArrayOop *JVM_DumpThreads(InstanceOop *_this, ObjArrayOop *threads);
void JVM_SetNativeThreadName(InstanceOop *_this, InstanceOop *str);


#endif /* INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_ */

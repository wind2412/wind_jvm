/*
 * java_lang_Thread.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

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

void JVM_StartThread(list<Oop *> & _stack);
void JVM_StopThread(list<Oop *> & _stack);
void JVM_IsThreadAlive(list<Oop *> & _stack);
void JVM_SuspendThread(list<Oop *> & _stack);
void JVM_ResumeThread(list<Oop *> & _stack);
void JVM_SetThreadPriority(list<Oop *> & _stack);
void JVM_Yield(list<Oop *> & _stack);
void JVM_Sleep(list<Oop *> & _stack);
void JVM_CurrentThread(list<Oop *> & _stack);
void JVM_CountStackFrames(list<Oop *> & _stack);
void JVM_Interrupt(list<Oop *> & _stack);
void JVM_IsInterrupted(list<Oop *> & _stack);
void JVM_HoldsLock(list<Oop *> & _stack);
void JVM_GetAllThreads(list<Oop *> & _stack);
void JVM_DumpThreads(list<Oop *> & _stack);
void JVM_SetNativeThreadName(list<Oop *> & _stack);

void *java_lang_thread_search_method(const wstring & str);

#endif /* INCLUDE_NATIVE_JAVA_LANG_THREAD_HPP_ */

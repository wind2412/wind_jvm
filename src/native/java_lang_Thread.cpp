/*
 * java_lang_Thread.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/jni.hpp"
#include "native/java_lang_Thread.hpp"
#include <vector>
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
void Java_java_lang_thread_registerNative(InstanceOop *_this)
{

}

void JVM_StartThread(InstanceOop *_this){assert(false);}
void JVM_StopThread(InstanceOop *_this, Oop *obj){assert(false);}
bool JVM_IsThreadAlive(InstanceOop *_this){assert(false);}
void JVM_SuspendThread(InstanceOop *_this){assert(false);}
void JVM_ResumeThread(InstanceOop *_this){assert(false);}
void JVM_SetThreadPriority(InstanceOop *_this, IntOop *){assert(false);}
void JVM_Yield(InstanceOop *_this){assert(false);}
void JVM_Sleep(InstanceOop *_this, LongOop *){assert(false);}
InstanceOop *JVM_CurrentThread(InstanceOop *_this){assert(false);}
IntOop *JVM_CountStackFrames(InstanceOop *_this){assert(false);}
void JVM_Interrupt(InstanceOop *_this){assert(false);}
bool JVM_IsInterrupted(InstanceOop *_this, BooleanOop *){assert(false);}
bool JVM_HoldsLock(InstanceOop *_this, InstanceOop *obj){assert(false);}
ObjArrayOop *JVM_GetAllThreads(InstanceOop *_this){assert(false);}
ObjArrayOop *JVM_DumpThreads(InstanceOop *_this, ObjArrayOop *threads){assert(false);}
void JVM_SetNativeThreadName(InstanceOop *_this, InstanceOop *str){assert(false);}

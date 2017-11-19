/*
 * java_lang_System.cpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_
#define SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_

#include "native/jni.hpp"
#include "native/java_lang_System.hpp"
#include <vector>
#include <cassert>

using std::vector;

static vector<JNINativeMethod> methods = {
	{"registerNatives",	 "()V",			         (void *)&Java_java_lang_system_registerNative},
    {"currentTimeMillis", "()J",                  (void *)&JVM_CurrentTimeMillis},
    {"nanoTime",          "()J",                  (void *)&JVM_NanoTime},
    {"arraycopy",         "(" OBJ "I" OBJ "II)V", (void *)&JVM_ArrayCopy},
    {"identityHashCode",  "(" OBJ ")I",           (void *)&JVM_IdentityHashCode},
    {"initProperties",    "(" PRO ")" PRO,        (void *)&JVM_InitProperties},
    {"mapLibraryName",    "(" STR ")" STR,        (void *)&JVM_MapLibraryName},
    {"setIn0",            "(" IPS ")V",           (void *)&JVM_SetIn0},
    {"setOut0",           "(" PRS ")V",           (void *)&JVM_SetOut0},
    {"setErr0",           "(" PRS ")V",           (void *)&JVM_SetErr0},
};

void Java_java_lang_system_registerNative(InstanceOop *_this)
{
		// 发现了一个问题：通过函数指针会把类型擦除！！因此估计要全部重改了...... 毕竟这如果要是擦除了，那就太糟糕了......
}

LongOop *JVM_CurrentTimeMillis(InstanceOop *_this){assert(false);}
LongOop *JVM_NanoTime(InstanceOop *_this){assert(false);}
void JVM_ArrayCopy(InstanceOop *_this, Oop *obj1, IntOop *i1, Oop *obj2, IntOop *i2, IntOop *i3){assert(false);}
IntOop *JVM_IdentityHashCode(InstanceOop *_this, Oop *obj){assert(false);}
InstanceOop *JVM_InitProperties(InstanceOop *_this, InstanceOop *prop){assert(false);}
InstanceOop *JVM_MapLibraryName(InstanceOop *_this, InstanceOop *str){assert(false);}
void JVM_SetIn0(InstanceOop *_this, InstanceOop *inputstream){assert(false);}
void JVM_SetOut0(InstanceOop *_this, InstanceOop *printstream){assert(false);}
void JVM_SetErr0(InstanceOop *_this, InstanceOop *printstream){assert(false);}

void java_lang_system_search_method(const string & str)
{
	for (int i = 0; i < methods.size(); i ++) {
		if ()
	}
}


#endif /* SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_ */

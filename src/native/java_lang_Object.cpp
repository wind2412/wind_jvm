/*
 * java_lang_Object.cpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#include "native/jni.hpp"
#include "native/java_lang_Object.hpp"
#include <vector>
#include <cassert>

using std::vector;

static vector<JNINativeMethod> methods = {
	{"registerNatives",	"()V",		  		 (void *)&Java_java_lang_object_registerNative},
    {"hashCode",    "()I",                    (void *)&JVM_IHashCode},
    {"wait",        "(J)V",                   (void *)&JVM_MonitorWait},
    {"notify",      "()V",                    (void *)&JVM_MonitorNotify},
    {"notifyAll",   "()V",                    (void *)&JVM_MonitorNotifyAll},
    {"clone",       "()" OBJ,                 (void *)&JVM_Clone},
    {"getClass",    "()" KLS,                 (void *)&Java_java_lang_object_getClass},		// I add one line here.
};

void Java_java_lang_object_registerNative(InstanceOop *_this)
{

}

IntOop *JVM_IHashCode(InstanceOop *_this){assert(false);}
void JVM_MonitorWait(InstanceOop *_this, LongOop *){assert(false);}
void JVM_MonitorNotify(InstanceOop *_this){assert(false);}
void JVM_MonitorNotifyAll(InstanceOop *_this){assert(false);}
Oop *JVM_Clone(InstanceOop *_this){assert(false);}
MirrorOop *Java_java_lang_object_getClass(InstanceOop *_this){assert(false);}

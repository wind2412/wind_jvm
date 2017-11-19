/*
 * java_lang_System.hpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_

#include "runtime/oop.hpp"

void Java_java_lang_system_registerNative(InstanceOop *_this);

LongOop *JVM_CurrentTimeMillis(InstanceOop *_this);
LongOop *JVM_NanoTime(InstanceOop *_this);
void JVM_ArrayCopy(InstanceOop *_this, Oop *obj1, IntOop *i1, Oop *obj2, IntOop *i2, IntOop *i3);
IntOop *JVM_IdentityHashCode(InstanceOop *_this, Oop *obj);
InstanceOop *JVM_InitProperties(InstanceOop *_this, InstanceOop *prop);
InstanceOop *JVM_MapLibraryName(InstanceOop *_this, InstanceOop *str);
void JVM_SetIn0(InstanceOop *_this, InstanceOop *inputstream);
void JVM_SetOut0(InstanceOop *_this, InstanceOop *printstream);
void JVM_SetErr0(InstanceOop *_this, InstanceOop *printstream);


#endif /* INCLUDE_NATIVE_JAVA_LANG_SYSTEM_HPP_ */

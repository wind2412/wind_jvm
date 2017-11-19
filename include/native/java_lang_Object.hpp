/*
 * java_lang_Object.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_
#define INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_

#include "runtime/oop.hpp"

void Java_java_lang_object_registerNative(InstanceOop *_this);

IntOop *JVM_IHashCode(InstanceOop *_this);
void JVM_MonitorWait(InstanceOop *_this, LongOop *);
void JVM_MonitorNotify(InstanceOop *_this);
void JVM_MonitorNotifyAll(InstanceOop *_this);
Oop *JVM_Clone(InstanceOop *_this);
MirrorOop *Java_java_lang_object_getClass(InstanceOop *_this);


#endif /* INCLUDE_NATIVE_JAVA_LANG_OBJECT_HPP_ */

/*
 * jni.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_NATIVE_HPP_
#define INCLUDE_NATIVE_NATIVE_HPP_

#include <string>
#include <unordered_map>
#include <functional>
#include <cassert>
#include <iostream>

using std::wstring;
using std::unordered_map;
using std::function;

// Java Native Interface Spec. [$ JNI types]
// ps: this jni is not real jni. It's target is only to support Native Method.

#define THD L"Ljava/lang/Thread;"
#define OBJ L"Ljava/lang/Object;"
#define STE L"Ljava/lang/StackTraceElement;"
#define STR L"Ljava/lang/String;"
#define IPS L"Ljava/io/InputStream;"
#define PRS L"Ljava/io/PrintStream;"
#define PRO L"Ljava/util/Properties;"
#define CLS L"Ljava/lang/Class;"
#define CPL "Lsun/reflect/ConstantPool;"
#define JCL "Ljava/lang/ClassLoader;"
#define FLD "Ljava/lang/reflect/Field;"
#define MHD "Ljava/lang/reflect/Method;"
#define CTR "Ljava/lang/reflect/Constructor;"
#define PD  "Ljava/security/ProtectionDomain;"
#define BA  "[B"

void init_native();

// 说明：所有的 native 方法我全部设置成为签名是 void (*)(list<Oop *> & _stack) 的形式了。避免了签名不同调用困难的问题。这样就很好啦～参数和返回值全都会被压入 _stack 中～～
void *find_native(const wstring & klass_name, const wstring & signature);	// get a native method <$signature> in klass <$klass_name>. maybe return nullptr...

#endif /* INCLUDE_NATIVE_NATIVE_HPP_ */

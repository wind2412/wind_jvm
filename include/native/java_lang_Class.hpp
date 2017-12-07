/*
 * java_lang_class.hpp
 *
 *  Created on: 2017年11月16日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_
#define INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_

#include <queue>
#include <list>
#include <unordered_map>
#include <string>
#include <cassert>
#include <memory>
#include "runtime/oop.hpp"

using std::queue;
using std::list;
using std::unordered_map;
using std::wstring;
using std::shared_ptr;

class MirrorOop;
class Klass;

class java_lang_class {
public:
	enum mirror_state { UnInited, Inited, Fixed };
public:
	static mirror_state & state();
	static queue<wstring> & get_single_delay_mirrors();
	static unordered_map<wstring, MirrorOop*> & get_single_basic_type_mirrors();
	static void init();		// must execute this method before jvm!!!
	static void fixup_mirrors();	// must execute this after java.lang.Class load!!!
	static MirrorOop *get_basic_type_mirror(const wstring & signature);	// "I", "Z", "D", "J" ......	// must execute this after `fixup_mirrors()` called !!
	static void if_Class_didnt_load_then_delay(shared_ptr<Klass> klass, MirrorOop *loader_mirror);
};

void JVM_GetClassName(list<Oop *> & _stack);
void JVM_ForClassName(list<Oop *> & _stack);
void JVM_GetSuperClass(list<Oop *> & _stack);
void JVM_GetClassInterfaces(list<Oop *> & _stack);
void JVM_GetClassLoader(list<Oop *> & _stack);
void JVM_IsInterface(list<Oop *> & _stack);
void JVM_IsInstance(list<Oop *> & _stack);
void JVM_IsAssignableFrom(list<Oop *> & _stack);
void JVM_GetClassSigners(list<Oop *> & _stack);
void JVM_SetClassSigners(list<Oop *> & _stack);
void JVM_IsArrayClass(list<Oop *> & _stack);
void JVM_IsPrimitiveClass(list<Oop *> & _stack);
void JVM_GetComponentType(list<Oop *> & _stack);
void JVM_GetClassModifiers(list<Oop *> & _stack);
void JVM_GetClassDeclaredFields(list<Oop *> & _stack);
void JVM_GetClassDeclaredMethods(list<Oop *> & _stack);
void JVM_GetClassDeclaredConstructors(list<Oop *> & _stack);
void JVM_GetProtectionDomain(list<Oop *> & _stack);
void JVM_GetDeclaredClasses(list<Oop *> & _stack);
void JVM_GetDeclaringClass(list<Oop *> & _stack);
void JVM_GetClassSignature(list<Oop *> & _stack);
void JVM_GetClassAnnotations(list<Oop *> & _stack);
void JVM_GetClassConstantPool(list<Oop *> & _stack);
void JVM_DesiredAssertionStatus(list<Oop *> & _stack);
void JVM_GetEnclosingMethodInfo(list<Oop *> & _stack);
void JVM_GetClassTypeAnnotations(list<Oop *> & _stack);
void JVM_GetPrimitiveClass(list<Oop *> & _stack);

void *java_lang_class_search_method(const wstring & signature);

#endif /* INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_ */

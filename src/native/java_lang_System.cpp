/*
 * java_lang_System.cpp
 *
 *  Created on: 2017年11月20日
 *      Author: zhengxiaolin
 */

#ifndef SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_
#define SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_

#include "native/java_lang_System.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "wind_jvm.hpp"

using std::vector;

static unordered_map<wstring, void*> methods = {
    {L"currentTimeMillis:()J",					(void *)&JVM_CurrentTimeMillis},
    {L"nanoTime:()J",							(void *)&JVM_NanoTime},
    {L"arraycopy:(" OBJ L"I" OBJ L"II)V",			(void *)&JVM_ArrayCopy},
    {L"identityHashCode:(" OBJ L")I",			(void *)&JVM_IdentityHashCode},
    {L"initProperties:(" PRO L")" PRO,			(void *)&JVM_InitProperties},
    {L"mapLibraryName:(" STR L")" STR,			(void *)&JVM_MapLibraryName},
    {L"setIn0:(" IPS L")V",						(void *)&JVM_SetIn0},
    {L"setOut0:(" PRS L")V",						(void *)&JVM_SetOut0},
    {L"setErr0:(" PRS L")V",						(void *)&JVM_SetErr0},
};

void JVM_CurrentTimeMillis(list<Oop *> & _stack)		// static
{
	assert(false);
}
void JVM_NanoTime(list<Oop *> & _stack){				// static
	assert(false);
}
void JVM_ArrayCopy(list<Oop *> & _stack){				// static
	Oop *obj1 = (Oop *)_stack.front();	_stack.pop_front();
	int src_pos = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	Oop *obj2 = (Oop *)_stack.front();	_stack.pop_front();
	int dst_pos = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int length = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	if (obj1 == obj2) {
		ArrayOop *objarr = (ArrayOop *)obj1;
		// memcpy has bugs so, we can't copy directly. => first alloc
		ArrayOop temp(*objarr);	// copy for one layer. deep copy its member variable for only one layer.
		for (int i = 0; i < length; i ++) {
			assert(src_pos + i < objarr->get_length() && dst_pos + i < objarr->get_length());	// TODO: ArrayIndexOutofBound
			(*objarr)[dst_pos + i] = temp[src_pos + i];
		}
	} else if (obj1->get_ooptype() == OopType::_TypeArrayOop) {
		assert(obj2->get_ooptype() == OopType::_TypeArrayOop);
		TypeArrayOop *objarr1 = (TypeArrayOop *)obj1;
		TypeArrayOop *objarr2 = (TypeArrayOop *)obj2;
		assert(objarr1->get_dimension() == objarr2->get_dimension());
		for (int i = 0; i < length; i ++) {
			assert(src_pos + i < objarr1->get_length() && dst_pos + i < objarr2->get_length());	// TODO: ArrayIndexOutofBound
			(*objarr2)[dst_pos + i] = (*objarr1)[src_pos + i];
		}
	} else if (obj1->get_ooptype() == OopType::_ObjArrayOop) {
		assert(obj2->get_ooptype() == OopType::_ObjArrayOop);
		ObjArrayOop *objarr1 = (ObjArrayOop *)obj1;
		ObjArrayOop *objarr2 = (ObjArrayOop *)obj2;
		assert(objarr1->get_dimension() == objarr2->get_dimension());
		assert(src_pos + length < objarr1->get_length() && dst_pos + length < objarr2->get_length());	// TODO: ArrayIndexOutofBound
		// 1. src has same element of dst, 2. or is sub_type of dst, 3. all the copy part are null.
		auto src_klass = std::static_pointer_cast<ObjArrayKlass>(objarr1->get_klass())->get_element_klass();
		auto dst_klass = std::static_pointer_cast<ObjArrayKlass>(objarr2->get_klass())->get_element_klass();
		if (src_klass == dst_klass || src_klass->check_parent(dst_klass) || src_klass->check_interfaces(dst_klass)) {		// 1 or 2
			// directly copy
			for (int i = 0; i < length; i ++) {
				(*objarr2)[dst_pos + i] = (*objarr1)[src_pos + i];
			}
		} else {
			bool splice_all_null = true;
			for (int i = 0; i < length; i ++) {
				if (!((*objarr1)[src_pos + i] == nullptr)) {
					splice_all_null = false;
					break;
				}
			}
			if (splice_all_null) {		// 3.
				for (int i = 0; i < length; i ++) {
					(*objarr2)[dst_pos + i] = nullptr;
				}
			} else {
				assert(false);
			}
		}
	} else {
		assert(false);
	}
#ifdef DEBUG
	std::wcout << "copy from objarray1[" << src_pos << "] to objarray2[" << dst_pos << "] for length: [" << length << "]." << std::endl;
#endif
}
void JVM_IdentityHashCode(list<Oop *> & _stack){		// static
	Oop *obj = (Oop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_InitProperties(list<Oop *> & _stack){		// static
	InstanceOop *prop = (InstanceOop *)_stack.front();	_stack.pop_front();
	wind_jvm & vm = *(wind_jvm *)_stack.back();	_stack.pop_back();
	shared_ptr<Method> hashtable_put = std::static_pointer_cast<InstanceKlass>(prop->get_klass())->get_class_method(L"put:(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	assert(hashtable_put != nullptr);
	// add properties: 	// this, key, value		// TODO: 没有设置完！！
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.specification.name"), java_lang_string::intern(L"Java Virtual Machine Specification")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.specification.version"), java_lang_string::intern(L"1.8")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.version"), java_lang_string::intern(L"25.0-b70-debug")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.name"), java_lang_string::intern(L"wind_jvm 64-Bit VM")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.info"), java_lang_string::intern(L"interpreted mode, sharing")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.specification.version"), java_lang_string::intern(L"1.8")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.version"), java_lang_string::intern(L"1.8.0-internal-debug")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.runtime.name"), java_lang_string::intern(L"windjvm Runtime Environment")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.runtime.version"), java_lang_string::intern(L"1.8.0")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"gopherProxySet"), java_lang_string::intern(L"false")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.vendor"), java_lang_string::intern(L"wind2412")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vendor.url"), java_lang_string::intern(L"http://wind2412.github.io/")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"path.separator"), java_lang_string::intern(L":")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.encoding.pkg"), java_lang_string::intern(L"sun.io")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.country"), java_lang_string::intern(L"CN")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.language"), java_lang_string::intern(L"zh")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.java.launcher"), java_lang_string::intern(L"WIND_STANDARD")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.os.patch.level"), java_lang_string::intern(L"unknown")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.arch"), java_lang_string::intern(L"x86_64")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.arch.data.model"), java_lang_string::intern(L"64")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"line.separator"), java_lang_string::intern(L"\n")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.separator"), java_lang_string::intern(L"/")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.jnu.encoding"), java_lang_string::intern(L"UTF-8")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.encoding"), java_lang_string::intern(L"UTF-8")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.specification.name"), java_lang_string::intern(L"Java Platform API Specification")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.version"), java_lang_string::intern(L"52.0")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.management.compiler"), java_lang_string::intern(L"nop")});
	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.io.unicode.encoding"), java_lang_string::intern(L"UnicodeBig")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.ext.dirs"), java_lang_string::intern(L"/Users/zhengxiaolin/Library/Java/Extensions:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib/ext:/Library/Java/Extensions:/Network/Library/Java/Extensions:/System/Library/Java/Extensions:/usr/lib/java")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.endorsed.dirs"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib/endorsed")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.boot.library.path"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.boot.class.path"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/resources.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/rt.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/sunrsasign.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jsse.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jce.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/charsets.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jfr.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/classes")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.library.path"), java_lang_string::intern(L"/Users/zhengxiaolin/Library/Java/Extensions:/Library/Java/Extensions:/Network/Library/Java/Extensions:/System/Library/Java/Extensions:/usr/lib/java:.")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vendor.url.bug"), java_lang_string::intern(L"http://bugreport.sun.com/bugreport/")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.home"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.name"), java_lang_string::intern(L"Mac OS X")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.path"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/eclipse/test1/bin")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.home"), java_lang_string::intern(L"/Users/zhengxiaolin")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.timezone"), java_lang_string::intern(L"Asia/Shanghai")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.version"), java_lang_string::intern(L"10.12.6")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.io.tmpdir"), java_lang_string::intern(L"/var/folders/wc/0c_zn09x12s_y90ccmvjb6900000gn/T/")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.dir"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/eclipse/test1")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.name"), java_lang_string::intern(L"zhengxiaolin")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.java.command"), java_lang_string::intern(L"test1.Test8")});
//	vm.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.cpu.endian"), java_lang_string::intern(L"little")});

	_stack.push_back(prop);
}
void JVM_MapLibraryName(list<Oop *> & _stack){		// static
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetIn0(list<Oop *> & _stack){		// static
	InstanceOop *inputstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetOut0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetErr0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

// 返回 fnPtr.
void *java_lang_system_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


#endif /* SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_ */

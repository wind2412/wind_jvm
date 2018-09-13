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
#include "classloader.hpp"
#include <sys/time.h>
#include "jarLister.hpp"

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
	timeval time;
	if (gettimeofday(&time, nullptr) == -1) {
		assert(false);
	}
	_stack.push_back(new LongOop(time.tv_sec * 1000 + time.tv_usec / 1000));		// second * 10^3 + u_second / 10^3 = micro_second.
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) now the nanotime is: [" << ((LongOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_NanoTime(list<Oop *> & _stack){				// static
	timeval time;
	if (gettimeofday(&time, nullptr) == -1) {
		assert(false);
	}
	_stack.push_back(new LongOop(time.tv_sec * 1000 * 1000 * 1000 + time.tv_usec * 1000));		// second * 10^9 + u_second * 10^3 = nano_second.
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) now the nanotime is: [" << ((LongOop *)_stack.back())->value << "]." << std::endl;
#endif
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

		function<InstanceKlass *(ObjArrayOop *)> get_ObjArray_real_element_klass_recursive = [&get_ObjArray_real_element_klass_recursive](ObjArrayOop *objarr) -> InstanceKlass *{
			int length = objarr->get_length();		// total length
			ObjArrayKlass * objarr_klass = ((ObjArrayKlass *)objarr->get_klass());		// this klass is not trusted. because it is the alloced array's obj_array_type, not the inner element type.
			if (length == 0) {
				// 1. if we can't find the inner-element, we can only return the objarr_klass's element-type... but thinking of the recursive function can return `objarr_klass->element_type()`, too, we return nullptr instead, it shows that we can't judge the **REAL** type.
//				return objarr_klass->get_element_klass();
				return nullptr;
			} else {
				for (int i = 0; i < length; i ++) {
					Oop *oop = (*objarr)[i];		// get the element.
					if (oop == nullptr) {
						// 2. if the inner is null:
						continue;
					} else {
						// 3. if not null: judge whether it is another ObjectArrayOop / InstanceOop.
						if (oop->get_ooptype() == OopType::_InstanceOop) {
							// 3.3. if InstanceOop, return it. This is the **REAL** type.
							return ((InstanceKlass *)oop->get_klass());
						} else {
							// 3.6. if ObjectArrayOop, then recursive.
							assert(oop->get_ooptype() == OopType::_ObjArrayOop);
							auto ret_klass = get_ObjArray_real_element_klass_recursive((ObjArrayOop *)oop);
							if (ret_klass != nullptr) {		// if the recursive get an answer:
								return ret_klass;
							} else							// if return nullptr, continue.
								continue;
						}
					}
				}
				// 4. all ObjArrayOop[0~length] are null.
				return nullptr;
			}
		};

		// an outer aux function.
		function<InstanceKlass *(ObjArrayOop *)> get_ObjArray_real_element_klass = [&get_ObjArray_real_element_klass_recursive](ObjArrayOop *objarr) -> InstanceKlass *{
			auto real_element_klass = get_ObjArray_real_element_klass_recursive(objarr);
			if (real_element_klass == nullptr) {
				// if it is nullptr after recursive, we can only consider it as the `alloc array`'s `objarr->element_type()`......
				return ((ObjArrayKlass *)objarr->get_klass())->get_element_klass();
			} else {
				return real_element_klass;
			}
		};

		assert(objarr1->get_dimension() == objarr2->get_dimension());
		assert(src_pos + length <= objarr1->get_length() && dst_pos + length <= objarr2->get_length());	// TODO: ArrayIndexOutofBound
		// 1. src has same element of dst, 2. or is sub_type of dst, 3. all the copy part are null.
		auto src_klass = get_ObjArray_real_element_klass(objarr1);
		auto dst_klass = get_ObjArray_real_element_klass(objarr1);

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
	sync_wcout{} << "copy from objarray1[" << src_pos << "] to objarray2[" << dst_pos << "] for length: [" << length << "]." << std::endl;
#endif
}
void JVM_IdentityHashCode(list<Oop *> & _stack){		// static
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	_stack.push_back(new IntOop((intptr_t)obj));
}
void JVM_InitProperties(list<Oop *> & _stack){		// static
	InstanceOop *prop = (InstanceOop *)_stack.front();	_stack.pop_front();
	vm_thread & thread = *(vm_thread *)_stack.back();	_stack.pop_back();
	Method *hashtable_put = ((InstanceKlass *)prop->get_klass())->get_class_method(L"put:(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	assert(hashtable_put != nullptr);

	// get pwd/sun_src
	wstring sun_src = pwd + L"/sun_src";

	// add properties: 	// this, key, value
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.specification.name"), java_lang_string::intern(L"Java Virtual Machine Specification")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.specification.version"), java_lang_string::intern(L"1.8")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.version"), java_lang_string::intern(L"25.0-b70-debug")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.name"), java_lang_string::intern(L"wind_jvm 64-Bit VM")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.info"), java_lang_string::intern(L"interpreted mode, sharing")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.specification.version"), java_lang_string::intern(L"1.8")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.version"), java_lang_string::intern(L"1.8.0-internal-debug")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.runtime.name"), java_lang_string::intern(L"windjvm Runtime Environment")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.runtime.version"), java_lang_string::intern(L"1.8.0")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"gopherProxySet"), java_lang_string::intern(L"false")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vm.vendor"), java_lang_string::intern(L"wind2412")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vendor.url"), java_lang_string::intern(L"http://wind2412.github.io/")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"path.separator"), java_lang_string::intern(L":")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.encoding.pkg"), java_lang_string::intern(L"sun.io")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.country"), java_lang_string::intern(L"CN")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.language"), java_lang_string::intern(L"zh")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.java.launcher"), java_lang_string::intern(L"WIND_STANDARD")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.os.patch.level"), java_lang_string::intern(L"unknown")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.arch"), java_lang_string::intern(L"x86_64")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.arch.data.model"), java_lang_string::intern(L"64")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"line.separator"), java_lang_string::intern(L"\n")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.separator"), java_lang_string::intern(L"/")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.jnu.encoding"), java_lang_string::intern(L"UTF-8")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"file.encoding"), java_lang_string::intern(L"UTF-8")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.specification.name"), java_lang_string::intern(L"Java Platform API Specification")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.version"), java_lang_string::intern(L"52.0")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.management.compiler"), java_lang_string::intern(L"nop")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.io.unicode.encoding"), java_lang_string::intern(L"UnicodeBig")});

	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.home"), java_lang_string::intern(pwd)});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.path"), java_lang_string::intern(pwd + L":" + sun_src)});		 // TODO: need modified.

	_stack.push_back(prop);
}
void JVM_MapLibraryName(list<Oop *> & _stack){		// static
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetIn0(list<Oop *> & _stack){		// static
	InstanceOop *inputstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	auto system = BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/System");
	assert(system != nullptr);
	((InstanceKlass *)system)->set_static_field_value(L"in:Ljava/io/InputStream;", inputstream);
}
void JVM_SetOut0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	auto system = BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/System");
	assert(system != nullptr);
	((InstanceKlass *)system)->set_static_field_value(L"out:Ljava/io/PrintStream;", printstream);
}
void JVM_SetErr0(list<Oop *> & _stack){		// static
	InstanceOop *printstream = (InstanceOop *)_stack.front();	_stack.pop_front();
	auto system = BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/System");
	assert(system != nullptr);
	((InstanceKlass *)system)->set_static_field_value(L"err:Ljava/io/PrintStream;", printstream);
}

void *java_lang_system_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}


#endif /* SRC_NATIVE_JAVA_LANG_SYSTEM_CPP_ */

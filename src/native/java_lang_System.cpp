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

//InstanceKlass *get_real_element_klass(ObjArrayKlass * obj_arr_klass)		// [x]见下文 ArrayCopy 中的 bug 说明。旨在对泛型擦除引发的 bug 打补丁。	// [x] 替换所有 get_element_klass！！甚至可以钦定此方法替换掉 get_element_klass!!
//{
//		// [x] 设计修改！成为：如果放置元素进去，那么就设置...
//		// [x] 再改！！因为泛型擦除只对 Object 成立......所以我如果看到 [Object，那就钦定......
//}

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
		// bug report！！这样，只能得到 alloc 的数组的数组类的 klass！而不能得到真正的内部存放的数值的 klass！！打个比方...比如 ArrayList 类。因为泛型擦除，因此内部有个 Object[] 数组......如果要是使用 get_element，就只能得到 inner 的类型是 Object...但是人家可能向内存放 String...... 唉。考虑不周...
		// 4. [x] 所以打算如果 src 是 java/lang/Object 的话，就改为检查数组内部的类型，并且加以钦定...
		// [√] 最终的方案改成了：一上来就得到真正的 element type 类型。
//		auto src_klass = ((ObjArrayKlass *)objarr1->get_klass())->get_element_klass();
//		auto dst_klass = ((ObjArrayKlass *)objarr2->get_klass())->get_element_klass();
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
				// [√] 下边的想法还是并不是很安全。所以改成了一上来就 get 到 real element type.
//				// 4. 走到这一步，也有可能是因为泛型擦除的关系。即 objarr1 的容器是 [Object，通过 get_element_klass 只能得到默认的“数组类型” Object，
//				// 却得不到内部存放的真实类型了。因此在这里，会判断内部的元素真实类型！而且由于 splice_all_null，我们知道 null 元素不存在！
//				// 给予最后一次机会：
//
//				if (src_klass->get_name() == L"java/lang/Object") {
//					// get `the most inner` element type!!
//					InstanceKlass *real_src_klass;
//					if (objarr1->get_dimension() == 1) {
//						// 如果一维 Object[]，且内部全不是 null，那么取出第一个就好～
//						if (length == 0) {
//							assert(false);	// I think it will be false...
//						} else {
//							real_src_klass = ((InstanceKlass *)(*objarr1)[src_pos]->get_klass());
//							if (real_src_klass == dst_klass || real_src_klass->check_parent(dst_klass) || real_src_klass->check_interfaces(dst_klass)) {
//								// directly copy
//								for (int i = 0; i < length; i ++) {
//									(*objarr2)[dst_pos + i] = (*objarr1)[src_pos + i];
//								}
//							} else {
//								assert(false);
//							}
//						}
//					} else {		// ...?
//						// 整理一下代码结构...写出一个可以判断真实类型的函数把...这样递归太恶心了...先 assert(false)了...
//						assert(false);
//
//					}
//				} else
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
	shared_ptr<Method> hashtable_put = ((InstanceKlass *)prop->get_klass())->get_class_method(L"put:(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	assert(hashtable_put != nullptr);
	// add properties: 	// this, key, value		// TODO: 没有设置完！！
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
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.reflect.noCaches"), java_lang_string::intern(L"true")});	// 直接魔改了（逃
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.ext.dirs"), java_lang_string::intern(L"/Users/zhengxiaolin/Library/Java/Extensions:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib/ext:/Library/Java/Extensions:/Network/Library/Java/Extensions:/System/Library/Java/Extensions:/usr/lib/java")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.endorsed.dirs"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib/endorsed")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.boot.library.path"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/jdk/lib")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.boot.class.path"), java_lang_string::intern(L"/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/resources.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/rt.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/sunrsasign.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jsse.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jce.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/charsets.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/lib/jfr.jar:/Users/zhengxiaolin/jvm/jdk8_mac/build/macosx-x86_64-normal-server-slowdebug/images/j2re-bundle/jre1.8.0.jre/Contents/Home/classes")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.library.path"), java_lang_string::intern(L"/Users/zhengxiaolin/Library/Java/Extensions:/Library/Java/Extensions:/Network/Library/Java/Extensions:/System/Library/Java/Extensions:/usr/lib/java:.")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.vendor.url.bug"), java_lang_string::intern(L"http://bugreport.sun.com/bugreport/")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.home"), java_lang_string::intern(L"/home/parallels/Documents/github/wind_jvm")});			// [x] 乱写的。不过不能不指定这个变量就好！因为 UnixFileSystem 中 canonicalize() 方法指明这个 java.home 不可以是 null，否则会崩溃。// [√] 也不能乱写...... java/util/Currency$1 的 run() 方法会在 $JAVA_HOME/lib/ 打开一个 currency.data 文件向内写入二进制文件......
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.home"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/github/wind_jvm")});			// [x] 乱写的。不过不能不指定这个变量就好！因为 UnixFileSystem 中 canonicalize() 方法指明这个 java.home 不可以是 null，否则会崩溃。// [√] 也不能乱写...... java/util/Currency$1 的 run() 方法会在 $JAVA_HOME/lib/ 打开一个 currency.data 文件向内写入二进制文件......
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.name"), java_lang_string::intern(L"Mac OS X")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.path"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/eclipse/test1/bin")});
	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.path"), java_lang_string::intern(L"/home/parallels/Documents/github/wind_jvm:/home/parallels/Documents/github/wind_jvm/sun_src")});		 // TODO: need modified.
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.class.path"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/github/wind_jvm:/Users/zhengxiaolin/Documents/github/wind_jvm/sun_src")});		 // TODO: need modified.
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.home"), java_lang_string::intern(L"/Users/zhengxiaolin")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.timezone"), java_lang_string::intern(L"Asia/Shanghai")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"os.version"), java_lang_string::intern(L"10.12.6")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"java.io.tmpdir"), java_lang_string::intern(L"/var/folders/wc/0c_zn09x12s_y90ccmvjb6900000gn/T/")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.dir"), java_lang_string::intern(L"/Users/zhengxiaolin/Documents/eclipse/test1")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"user.name"), java_lang_string::intern(L"zhengxiaolin")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.java.command"), java_lang_string::intern(L"test1.Test8")});
//	thread.add_frame_and_execute(hashtable_put, {prop, java_lang_string::intern(L"sun.cpu.endian"), java_lang_string::intern(L"little")});

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

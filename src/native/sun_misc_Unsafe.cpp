/*
 * sun_misc_Unsafe.cpp
 *
 *  Created on: 2017年11月22日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_Unsafe.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include "utils/os.hpp"
#include "classloader.hpp"
#include "wind_jvm.hpp"

static unordered_map<wstring, void*> methods = {
    {L"arrayBaseOffset:(" CLS ")I",							(void *)&JVM_ArrayBaseOffset},
    {L"arrayIndexScale:(" CLS ")I",							(void *)&JVM_ArrayIndexScale},
    {L"addressSize:()I",										(void *)&JVM_AddressSize},
    {L"objectFieldOffset:(" FLD ")J",						(void *)&JVM_ObjectFieldOffset},
    {L"getIntVolatile:(" OBJ "J)I",							(void *)&JVM_GetIntVolatile},
    {L"compareAndSwapInt:(" OBJ "JII)Z",						(void *)&JVM_CompareAndSwapInt},
    {L"allocateMemory:(J)J",									(void *)&JVM_AllocateMemory},
    {L"putLong:(JJ)V",										(void *)&JVM_PutLong},
    {L"getByte:(J)B",										(void *)&JVM_GetByte},
    {L"freeMemory:(J)V",										(void *)&JVM_FreeMemory},
    {L"getObjectVolatile:(" OBJ "J)" OBJ ,					(void *)&JVM_GetObjectVolatile},
    {L"compareAndSwapObject:(" OBJ "J" OBJ OBJ ")Z",			(void *)&JVM_CompareAndSwapObject},
    {L"compareAndSwapLong:(" OBJ "JJJ)Z",						(void *)&JVM_CompareAndSwapLong},
    {L"shouldBeInitialized:(" CLS ")Z",						(void *)&JVM_ShouldBeInitialized},
    {L"defineAnonymousClass:(" CLS "[B[" OBJ ")" CLS,			(void *)&JVM_DefineAnonymousClass},
    {L"ensureClassInitialized:(" CLS ")V",					(void *)&JVM_EnsureClassInitialized},
    {L"defineClass:(" STR "[BII" JCL PD ")" CLS,				(void *)&JVM_DefineClass},
    {L"putObjectVolatile:(" OBJ "J" OBJ ")V",					(void *)&JVM_PutObjectVolatile},
    {L"staticFieldOffset:(" FLD ")J",						(void *)&JVM_StaticFieldOffset},
    {L"staticFieldBase:(" FLD ")" OBJ,						(void *)&JVM_StaticFieldBase},
    {L"putObject:(" OBJ "J" OBJ ")V",						(void *)&JVM_PutObject},
};

void JVM_ArrayBaseOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
#ifdef DEBUG
	sync_wcout{} << "[arrayBaseOffset] " << _array->get_buf_offset() << std::endl;		// delete
#endif
	_stack.push_back(new IntOop(_array->get_buf_offset()));
}

void JVM_ArrayIndexScale(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	ArrayOop *_array = (ArrayOop *)_stack.front();	_stack.pop_front();
#ifdef DEBUG
	sync_wcout{} << "[arrayScaleOffset] " << sizeof(intptr_t) << std::endl;		// delete
#endif
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}

void JVM_AddressSize(list<Oop *> & _stack){
	_stack.push_back(new IntOop(sizeof(intptr_t)));
}

// see: http://hllvm.group.iteye.com/group/topic/37940
void JVM_ObjectFieldOffset(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *field = (InstanceOop *)_stack.front();	_stack.pop_front();	// java/lang/reflect/Field obj.

	Oop *oop;
	assert(field->get_field_value(FIELD L":modifiers:I", &oop));
	int modifier = ((IntOop *)oop)->value;

	// static 的方面，在 JVM_StaticFieldOffset 中支持！
	assert(field->get_field_value(FIELD L":name:Ljava/lang/String;", &oop));
	wstring name = java_lang_string::stringOop_to_wstring((InstanceOop *)oop);

	assert(field->get_field_value(FIELD L":type:Ljava/lang/Class;", &oop));
	MirrorOop *mirror = (MirrorOop *)oop;

	wstring descriptor = name + L":";

	// get the Type of this member variable.	// maybe: BasicType, InstanceType, ArrayType{ObjArrayType, BasicArrayType}.
	shared_ptr<Klass> mirrored_who = mirror->get_mirrored_who();
	if (mirrored_who) {	// not primitive type
		if (mirrored_who->get_type() == ClassType::InstanceClass) {
			descriptor += (L"L" + mirrored_who->get_name() + L";");
		} else if (mirrored_who->get_type() == ClassType::ObjArrayClass) {
			descriptor += mirrored_who->get_name();
		} else if (mirrored_who->get_type() == ClassType::TypeArrayClass) {
			descriptor += mirrored_who->get_name();
		} else {
			assert(false);
		}
	} else {
		assert(mirror->get_extra() != L"");
		descriptor += mirror->get_extra();
	}

	// get the class which has the member variable.
	assert(field->get_field_value(FIELD L":clazz:Ljava/lang/Class;", &oop));
	MirrorOop *outer_klass_mirror = (MirrorOop *)oop;
	assert(outer_klass_mirror->get_mirrored_who()->get_type() == ClassType::InstanceClass);	// outer must be InstanceType.
	shared_ptr<InstanceKlass> outer_klass = std::static_pointer_cast<InstanceKlass>(outer_klass_mirror->get_mirrored_who());

	wstring BIG_signature = outer_klass->get_name() + L":" + descriptor;

	int offset = outer_klass->get_all_field_offset(BIG_signature);

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) the field which names [ " << BIG_signature << " ], inside the [" << outer_klass->get_name() << "], has the offset [" << offset << "] of its FIELDS." << std::endl;
#endif

	_stack.push_back(new LongOop(offset));
}

void JVM_GetIntVolatile(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	// I don't know...
	assert(obj->get_klass()->get_type() == ClassType::InstanceClass);

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) [dangerous] will get an int from obj oop:[" << obj << "], which klass_name is: [" <<
			std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_name() << "], offset: [" << offset << "]: " << std::endl;
#endif
	Oop *target;
	if (std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num() <= offset) {		// it's encoded static field offset.
		offset -= std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num();	// decode
		target = std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_static_fields_addr()[offset];
	} else {		// it's in non-static field.
		target = obj->get_fields_addr()[offset];
	}
	// 非常危险...
	assert(target->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)target)->get_type() == Type::INT);

	int value = *((volatile int *)&((volatile IntOop *)target)->value);		// volatile
	_stack.push_back(new IntOop(value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ---> int value is [" << ((IntOop *)target)->value << "] " << std::endl;
#endif

}

Oop **get_inner_oop_from_instance_oop_of_static_or_non_static_fields(InstanceOop *obj, long offset)
{
	Oop **target;
	if (std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num() <= offset) {		// it's encoded static field offset.
		offset -= std::static_pointer_cast<InstanceKlass>(obj->get_klass())->non_static_field_num();	// decode
		target = &std::static_pointer_cast<InstanceKlass>(obj->get_klass())->get_static_fields_addr()[offset];
	} else {		// it's in non-static field.
		target = &obj->get_fields_addr()[offset];
	}
	return target;
}

void JVM_CompareAndSwapInt(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	int expected = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int x = ((IntOop *)_stack.front())->value;	_stack.pop_front();

	Oop **target = get_inner_oop_from_instance_oop_of_static_or_non_static_fields(obj, offset);

	assert((*target)->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)*target)->get_type() == Type::INT);

	// CAS, from x86 assembly, and openjdk.
	_stack.push_back(new IntOop(cmpxchg(x, &((IntOop *)*target)->value, expected) == expected));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) compare obj + offset with [" << expected << "] and swap to be [" << x << "], success: [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_AllocateMemory(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long size = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	if (size < 0) {
		assert(false);
	} else if (size == 0) {
		_stack.push_back(new LongOop((uintptr_t)nullptr));
	} else {
		void *addr = ::malloc(size);														// TODO: GC !!!!! 这个是堆外分配！！
		_stack.push_back(new LongOop((uintptr_t)addr));		// 地址放入。不过会转成 long。
	}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) malloc size of [" << size << "] at address: [" << std::hex << ((LongOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_PutLong(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	long val = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	*((long *)addr) = val;

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) put long val [" << val << "] at address: [" << std::hex << addr << "]." << std::endl;
#endif
}

void JVM_GetByte(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	uint8_t val = *((uint8_t *)addr);

	_stack.push_back(new IntOop(val));

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get byte val [" << std::dec << val << "] at address: [" << std::hex << addr << "]." << std::endl;
#endif
}

void JVM_FreeMemory(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	long addr = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	::free((void *)addr);

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) free Memory of address: [" << std::hex << addr << "]." << std::endl;
#endif
}

void *get_inner_obj_from_obj_and_offset(Oop *obj, long offset)		// obj 可能是一个 InstanceOop，也有可能是一个 ArrayOop。
{
	void *addr;

	// 在这里我的实现 hack 了一发～～～ 由于我的 oop 对象模型中，InstanceOop 的 field 全是挂在堆分配的内存中的。所以 Unsafe 得到的 offset 是 field [之内] 的 offset，也就是 field 内部的一个 oop 相对于 field 这个 vector 本身的地址。
	// 而并不是 openjdk 实现中的相对于 this 的 offset。由于设计不同，只有这样才能让我日后的 GC 能够选择更多的算法。但是针对 ArrayOop，这样不行。因为 ArrayOop 的 Unsafe 已经写死了 ——
	// 是通过 ABASE + i << ASHIFT (java/util/concurrent/ConcurrentHashMap) 来实现的 真·偏移。所以，看到这一点之后，我把 ArrayBaseOffset 默认返回了 0.
	// 这样，每次传进来的此方法的 offset，就一定只是 i << ASHIFT。即 i * sizeof(intptr_t)。然后就可以逆向算出来 i 的大小～
	// 而且，在下边为了 InstanceOop 和 ArrayOop 区别对待，需要判断 obj 的类型～

	if (obj->get_ooptype() == OopType::_TypeArrayOop || obj->get_ooptype() == OopType::_ObjArrayOop) {
		// 通过 vector 相对偏移来取值。直接 volatile + barrier 取值就可以。
		int i_element = offset / sizeof(intptr_t);
		addr = (void *)&((*(ArrayOop *)obj)[i_element]);		// Oop ** to void *.
		if (*(Oop **)addr != nullptr)
			assert((*(Oop **)addr)->get_ooptype() == OopType::_BasicTypeOop || (*(Oop **)addr)->get_ooptype() == OopType::_InstanceOop || (*(Oop **)addr)->get_ooptype() == OopType::_TypeArrayOop || (*(Oop **)addr)->get_ooptype() == OopType::_ObjArrayOop);
	} else if (obj->get_ooptype() == OopType::_InstanceOop) {
		// 也是通过 vector 相对偏移来取值～
		Oop **target = get_inner_oop_from_instance_oop_of_static_or_non_static_fields((InstanceOop *)obj, offset);
		// 非常危险...
		if (*target != nullptr)
			assert((*target)->get_ooptype() == OopType::_InstanceOop);		// 其实 inner 可能是任意类型吧....等到用到再改......
		addr = (void *)target;
	} else {
		assert(false);
	}

	return addr;
}

void JVM_GetObjectVolatile(list<Oop *> & _stack){			// volatile + memory barrier!!!
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	Oop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	void *addr = get_inner_obj_from_obj_and_offset(obj, offset);

	// the code's thought is from openjdk:
	volatile Oop * v = *(volatile Oop **)addr;		// TODO: 这里还有不理解的地方......等待高人解惑......

	acquire();

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get an Oop from address: [" << std::hex << addr << "]." << std::endl;
#endif

	_stack.push_back(const_cast<Oop *&>(v));		// get rid of cv constrait symbol.
}

void JVM_CompareAndSwapObject(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *expected = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *x = (InstanceOop *)_stack.front();	_stack.pop_front();

	void *addr = get_inner_obj_from_obj_and_offset(obj, offset);


	// CAS, from x86 assembly, and openjdk.
	_stack.push_back(new IntOop(cmpxchg((long)x, (volatile long *)addr, (long)expected) == (long)expected));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) compare obj + offset with [" << expected << "] and swap to be [" << x << "], success: [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_CompareAndSwapLong(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	long expected = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	long x = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	Oop **target = get_inner_oop_from_instance_oop_of_static_or_non_static_fields(obj, offset);

	assert((*target)->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)*target)->get_type() == Type::LONG);

	// CAS, from x86 assembly, and openjdk.
	_stack.push_back(new IntOop(cmpxchg(x, &((LongOop *)*target)->value, expected) == expected));

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) compare obj + offset with [" << expected << "] and swap to be [" << x << "], success: [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_ShouldBeInitialized(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	MirrorOop *klass = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(klass->get_mirrored_who() != nullptr);
	_stack.push_back(new IntOop(klass->get_mirrored_who()->get_state() == Klass::KlassState::NotInitialized));
}

void JVM_DefineAnonymousClass(list<Oop *> & _stack){	// see: OpenJDK: unsafe.cpp:Unsafe_DefineAnonymousClass_impl().
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	MirrorOop *host_klass_mirror = (MirrorOop *)_stack.front();	_stack.pop_front();			// hostclass. see java sourcecode backtrace can see, It can get from: @CallerSensitive: getCallerClass.
	TypeArrayOop *bytes = (TypeArrayOop *)_stack.front();	_stack.pop_front();		// bytecodes.
	ObjArrayOop *cp_patch = (ObjArrayOop *)_stack.front();	_stack.pop_front();		// cp_patch array, maybe null.

//	assert(bytes->get_length() > offset && bytes->get_length() >= (offset + len));		// ArrayIndexOutofBoundException

	int len = bytes->get_length();

	// delete all:	( 调试信息还是留着吧. )
//	vm_thread *thread = (vm_thread *)_stack.back();		// 输出堆栈信息
//	thread->get_stack_trace();			// delete
//	// delete:
//	for (int i = 0; i < bytes->get_length(); i ++) {		// 输出全部字节码
//		std::wcout << std::hex << ((IntOop *)(*bytes)[i])->value << " " << std::dec;
//	}
//	std::wcout << std::endl;
//	std::wcout << host_klass_mirror->get_mirrored_who()->get_name() << " " << len << " " << cp_patch->get_length() << std::endl;	// delete


	wstring klass_name = L"<unknown>";

	char *buf = new char[len];

	for (int i = 0; i < len; i ++) {
		buf[i] = (char)((IntOop *)(*bytes)[i])->value;
	}

	ByteStream byte_buf(buf, len);

	// host_klass
	shared_ptr<InstanceKlass> host_klass = std::static_pointer_cast<InstanceKlass>(host_klass_mirror->get_mirrored_who());
	// host_klass's java loader.
	MirrorOop *host_klass_loader = (host_klass == nullptr) ? nullptr : std::static_pointer_cast<InstanceKlass>(host_klass)->get_java_loader();

	auto anonymous_klass = MyClassLoader::get_loader().loadClass(klass_name, &byte_buf, host_klass_loader,
																 true, host_klass, cp_patch);
	assert(anonymous_klass != nullptr);
	_stack.push_back(anonymous_klass->get_mirror());

	delete[] buf;

}

void JVM_EnsureClassInitialized(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	MirrorOop *klass = (MirrorOop *)_stack.front();	_stack.pop_front();
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();

	assert(klass->get_mirrored_who()->get_type() == ClassType::InstanceClass);
	auto real_klass = std::static_pointer_cast<InstanceKlass>(klass->get_mirrored_who());
	assert(real_klass != nullptr);
	BytecodeEngine::initial_clinit(real_klass, *thread);
}

void JVM_DefineClass(list<Oop *> & _stack){
	// same as: java_lang_ClassLoader.hpp::JVM_DefineClass1().
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *name = (InstanceOop *)_stack.front();	_stack.pop_front();
	TypeArrayOop *bytes = (TypeArrayOop *)_stack.front();	_stack.pop_front();
	int offset = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	int len = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *loader = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *protection_domain = (InstanceOop *)_stack.front();	_stack.pop_front();		// 我忽略掉这东西了。安全机制就算了。

	assert(bytes->get_length() > offset && bytes->get_length() >= (offset + len));		// ArrayIndexOutofBoundException

	wstring klass_name = java_lang_string::stringOop_to_wstring(name);

	char *buf = new char[len];

	for (int i = offset, j = 0; i < offset + len; i ++, j ++) {
		buf[j] = (char)((IntOop *)(*bytes)[i])->value;
	}

	ByteStream byte_buf(buf, len);

	// 和 java_lang_ClassLoader.hpp::JVM_DefineClass1() 不同的是...... 这个 loader 是传进来的参数，可能是 null；而那个的 loader 是 _this，不可能是 null...
//	assert(loader != nullptr);
	// 嗯。看来唯一的不同就是，这里的 loader 可以是 nullptr....

	shared_ptr<Klass> klass;
	if (loader != nullptr) {
		klass = MyClassLoader::get_loader().loadClass(klass_name, &byte_buf, std::static_pointer_cast<InstanceKlass>(loader->get_klass())->get_java_loader());
	} else {
		klass = MyClassLoader::get_loader().loadClass(klass_name, &byte_buf, nullptr);
	}
	assert(klass != nullptr);
	_stack.push_back(klass->get_mirror());

	delete[] buf;

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) DefineClass1(): [" << klass_name << "]." << std::endl;
#endif
}

void JVM_PutObjectVolatile(list<Oop *> & _stack){		// put `target` at obj[offset].
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *target = (InstanceOop *)_stack.front();	_stack.pop_front();

	assert(obj != nullptr);

	void *addr = get_inner_obj_from_obj_and_offset(obj, offset);		// really Oop **.

	release();

	Oop *temp = *(Oop **)addr;
	*(Oop **)addr = target;

	fence();


#ifdef DEBUG
	wstring target_name = (target != nullptr) ? target->get_klass()->get_name() : L"null";
	wstring addr_name = (temp != nullptr) ? temp->get_klass()->get_name() : L"null";
	sync_wcout{} << "(DEBUG) put an Oop, which is [" << target_name << "], to address: [" << std::hex << *(Oop **)addr << "], which is the type [" << addr_name << "]." << std::endl;
#endif

}

// totally same as: JVM_ObjectFieldOffset
void JVM_StaticFieldOffset(list<Oop *> & _stack){
	JVM_ObjectFieldOffset(_stack);
}

void JVM_StaticFieldBase(list<Oop *> & _stack){
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *field = (InstanceOop *)_stack.front();	_stack.pop_front();

	Oop *oop;
	field->get_field_value(FIELD L":clazz:" CLS, &oop);
	MirrorOop *mirror = (MirrorOop *)oop;

	field->get_field_value(FIELD L":modifiers:I", &oop);
	assert((((IntOop *)oop)->value & ACC_STATIC) == ACC_STATIC);

	_stack.push_back(mirror);

	// only return its mirror????

}

void JVM_PutObject(list<Oop *> & _stack){			// (no volatile!!)
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();
	long offset = ((LongOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *target = (InstanceOop *)_stack.front();	_stack.pop_front();

	assert(obj != nullptr);

	void *addr = get_inner_obj_from_obj_and_offset(obj, offset);		// really Oop **.

	Oop *temp = *(Oop **)addr;
	*(Oop **)addr = target;

#ifdef DEBUG
	wstring target_name = (target != nullptr) ? target->get_klass()->get_name() : L"null";
	wstring addr_name = (temp != nullptr) ? temp->get_klass()->get_name() : L"null";
	sync_wcout{} << "(DEBUG) put an Oop, which is [" << target_name << "], to address: [" << std::hex << *(Oop **)addr << "], which is the type [" << addr_name << "]." << std::endl;
#endif
}

// 返回 fnPtr.
void *sun_misc_unsafe_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

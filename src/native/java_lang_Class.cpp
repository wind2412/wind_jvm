/*
 * java_lang_class.cpp
 *
 *  Created on: 2017年11月16日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Class.hpp"
#include "native/java_lang_String.hpp"
#include "runtime/klass.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"
#include "native/native.hpp"
#include "wind_jvm.hpp"

/*===--------------------- java_lang_class ----------------------===*/
java_lang_class::mirror_state & java_lang_class::state() {
	static mirror_state state = UnInited;
	return state;
}

queue<wstring> & java_lang_class::get_single_delay_mirrors() {
	static queue<wstring> delay_mirrors;	// the Klass's name (compatible with basic type: I,Z,D,J...) which is parsed before java.lang.Class parsed. It will be added into the queue.
	return delay_mirrors;
}

unordered_map<wstring, MirrorOop*> & java_lang_class::get_single_basic_type_mirrors() {
	static unordered_map<wstring, MirrorOop*> basic_type_mirrors;		// a map to restore basic type mirrors. Such as `int`, `long`...。 `int[]`'s mirror will be `int`'s.
	return basic_type_mirrors;
}

void java_lang_class::init() {		// must execute this method before jvm!!!
	auto & delay_mirrors = get_single_delay_mirrors();
	// basic types.
	delay_mirrors.push(L"I");
	delay_mirrors.push(L"Z");
	delay_mirrors.push(L"B");
	delay_mirrors.push(L"C");
	delay_mirrors.push(L"S");
	delay_mirrors.push(L"F");
	delay_mirrors.push(L"J");
	delay_mirrors.push(L"D");
	delay_mirrors.push(L"[I");
	delay_mirrors.push(L"[Z");
	delay_mirrors.push(L"[B");
	delay_mirrors.push(L"[C");
	delay_mirrors.push(L"[S");
	delay_mirrors.push(L"[F");
	delay_mirrors.push(L"[J");
	delay_mirrors.push(L"[D");
	// set state
	state() = Inited;
}

void java_lang_class::fixup_mirrors() {	// must execute this after java.lang.Class load!!!
	assert(state() == Inited);
	assert(system_classmap.find(L"java/lang/Class.class") != system_classmap.end());		// java.lang.Class must be loaded !!
	// set state
	state() = Fixed;
	// do fix-up
	auto & delay_mirrors = get_single_delay_mirrors();		// 别忘了加上 & ！！否则会复制！！auto 的类型推导仅仅能推导出类型，但是不会给你加上引用！！！
	while(!delay_mirrors.empty()) {
		wstring name = delay_mirrors.front();
		delay_mirrors.pop();

			std::wcout << " fix up..." << name << std::endl;	// delete

		shared_ptr<Klass> klass = system_classmap.find(L"java/lang/Class.class")->second;
		if (name.size() == 1)	// ... switch only accept an integer... can't accept a wstring.
			switch (name[0]) {
				case L'I':case L'Z':case L'B':case L'C':case L'S':case L'F':case L'J':case L'D':{
					// insert into.
					get_single_basic_type_mirrors().insert(make_pair(name, std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(nullptr, nullptr)));
					break;
				}
				default:{
					assert(false);
				}
			}
		else if (name.size() == 2 && name[0] == L'[') {
			switch (name[1]) {
				case L'I':case L'Z':case L'B':case L'C':case L'S':case L'F':case L'J':case L'D':{
					auto arr_klass = BootStrapClassLoader::get_bootstrap().loadClass(name);		// load the simple array klass first.
					auto basic_type_mirror_iter = get_single_basic_type_mirrors().find(wstring(1, name[1]));
					assert(basic_type_mirror_iter != get_single_basic_type_mirrors().end());
					std::static_pointer_cast<TypeArrayKlass>(arr_klass)->set_mirror((*basic_type_mirror_iter).second);
					break;
				}
				default:{
					assert(false);
				}
			}
		}
		else {
			// I set java.lang.Class load at the first of jvm. So there can't be any user-loaded-klass. So find in the system_map.
			auto iter = system_classmap.find(name);
			assert(iter != system_classmap.end());
			assert((*iter).second->get_mirror() == nullptr);
			(*iter).second->set_mirror(std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(std::static_pointer_cast<InstanceKlass>((*iter).second), nullptr));
		}
	}
}

MirrorOop *java_lang_class::get_basic_type_mirror(const wstring & signature) {	// "I", "Z", "D", "J" ......	// must execute this after `fixup_mirrors()` called !!
	assert(state() == Fixed);
	auto & basic_type_mirrors = get_single_basic_type_mirrors();
	unordered_map<wstring, MirrorOop*>::iterator iter;
	if ((iter = basic_type_mirrors.find(signature)) != basic_type_mirrors.end()) {
		return (*iter).second;
	}
	assert(iter != basic_type_mirrors.end());
	return nullptr;
}

void java_lang_class::if_Class_didnt_load_then_delay(shared_ptr<Klass> klass, ClassLoader *loader) {
	// set java_mirror
	// this if only for Primitive Array/Primitive Type.
	if (java_lang_class::state() != java_lang_class::Fixed) {	// java.lang.Class not loaded... delay it.
		if (klass->get_type() == ClassType::InstanceClass)
			java_lang_class::get_single_delay_mirrors().push(klass->get_name() + L".class");
//			else if (klass->get_type() == ClassType::TypeArrayClass)	// has been delayed in `java_lang_class::init()`.
		else if (klass->get_type() == ClassType::ObjArrayClass) {		// maybe deprecated.
			assert(false);
//				auto delayed_queue = get_single_delay_mirrors();
//				if (delayed_queue)
//				java_lang_class::get_single_delay_mirrors().push(std::static_pointer_cast<Obj>() + L".class");
		} else {
			assert(false);
		}
	} else {
		if (klass->get_type() == ClassType::InstanceClass) {
			klass->set_mirror(std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(std::static_pointer_cast<InstanceKlass>(klass), loader));	// set java_mirror
		}
		else if (klass->get_type() == ClassType::TypeArrayClass) {
			switch (std::static_pointer_cast<TypeArrayKlass>(klass)->get_basic_type()) {
				case Type::BOOLEAN:
					klass->set_mirror(get_basic_type_mirror(L"Z"));	// set java_mirror
					break;
				case Type::INT:
					klass->set_mirror(get_basic_type_mirror(L"I"));	// set java_mirror
					break;
				case Type::FLOAT:
					klass->set_mirror(get_basic_type_mirror(L"F"));	// set java_mirror
					break;
				case Type::DOUBLE:
					klass->set_mirror(get_basic_type_mirror(L"D"));	// set java_mirror
					break;
				case Type::LONG:
					klass->set_mirror(get_basic_type_mirror(L"J"));	// set java_mirror
					break;
				case Type::SHORT:
					klass->set_mirror(get_basic_type_mirror(L"S"));	// set java_mirror
					break;
				case Type::BYTE:
					klass->set_mirror(get_basic_type_mirror(L"B"));	// set java_mirror
					break;
				case Type::CHAR:
					klass->set_mirror(get_basic_type_mirror(L"C"));	// set java_mirror
					break;
				default:{
					assert(false);
				}
			}
			assert(klass->get_mirror() != nullptr);
		} else if (klass->get_type() == ClassType::ObjArrayClass) {
			klass->set_mirror(std::static_pointer_cast<ObjArrayKlass>(klass)->get_element_klass()->get_mirror());		// set to element class's mirror!
		} else {
			assert(false);
		}
	}
}

/*===----------------------- Natives ----------------------------===*/
static unordered_map<wstring, void*> methods = {
    {L"getName0:()" STR,						(void *)&JVM_GetClassName},
    {L"forName0:(" STR L"Z" JCL CLS ")" CLS,	(void *)&JVM_ForClassName},
//  {L"getSuperclass:()" CLS,				NULL},			// 为啥是 NULL ？？？
    {L"getSuperclass:()" CLS,				(void *)&JVM_GetSuperClass},			// 那我可自己实现了...
    {L"getInterfaces0:()[" CLS,				(void *)&JVM_GetClassInterfaces},
    {L"getClassLoader0:()" JCL,				(void *)&JVM_GetClassLoader},
    {L"isInterface:()Z",						(void *)&JVM_IsInterface},
    {L"isInstance:(" OBJ ")Z",				(void *)&JVM_IsInstance},
    {L"isAssignableFrom:(" CLS ")Z",			(void *)&JVM_IsAssignableFrom},
    {L"getSigners:()[" OBJ,					(void *)&JVM_GetClassSigners},
    {L"setSigners:([" OBJ ")V",				(void *)&JVM_SetClassSigners},
    {L"isArray:()Z",							(void *)&JVM_IsArrayClass},
    {L"isPrimitive:()Z",						(void *)&JVM_IsPrimitiveClass},
    {L"getComponentType:()" CLS,				(void *)&JVM_GetComponentType},
    {L"getModifiers:()I",					(void *)&JVM_GetClassModifiers},
    {L"getDeclaredFields0:(Z)[" FLD,			(void *)&JVM_GetClassDeclaredFields},
    {L"getDeclaredMethods0:(Z)[" MHD,		(void *)&JVM_GetClassDeclaredMethods},
    {L"getDeclaredConstructors0:(Z)[" CTR,	(void *)&JVM_GetClassDeclaredConstructors},
    {L"getProtectionDomain0:()" PD,			(void *)&JVM_GetProtectionDomain},
    {L"getDeclaredClasses0:()[" CLS,			(void *)&JVM_GetDeclaredClasses},
    {L"getDeclaringClass0:()" CLS,			(void *)&JVM_GetDeclaringClass},
    {L"getGenericSignature0:()" STR,			(void *)&JVM_GetClassSignature},
    {L"getRawAnnotations:()" BA,				(void *)&JVM_GetClassAnnotations},
    {L"getConstantPool:()" CPL,				(void *)&JVM_GetClassConstantPool},
    {L"desiredAssertionStatus0:(" CLS ")Z",	(void *)&JVM_DesiredAssertionStatus},
    {L"getEnclosingMethod0:()[" OBJ,			(void *)&JVM_GetEnclosingMethodInfo},
    {L"getRawTypeAnnotations:()" BA,			(void *)&JVM_GetClassTypeAnnotations},
    {L"getPrimitiveClass:(" STR ")" CLS,		(void *)&JVM_GetPrimitiveClass},
};

// TODO: 调查他们哪个是 static！！

void JVM_GetClassName(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(_this != nullptr);
	Oop *str = java_lang_string::intern(_this->get_mirrored_who()->get_name());
#ifdef DEBUG
	std::wcout << "(DEBUG) native method [java/lang/Class.getName0()] get `this` classname: [" << java_lang_string::stringOop_to_wstring((InstanceOop *)str) << "]." << std::endl;
#endif
	_stack.push_back(str);
}
void JVM_ForClassName(list<Oop *> & _stack){		// static
	assert(false);	// 不知道能不能用的上
	wind_jvm & vm = *(wind_jvm *)_stack.back();	_stack.pop_back();
	wstring klass_name = java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.front());	_stack.pop_front();
	bool initialize = ((BooleanOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *loader = (InstanceOop *)_stack.front();	_stack.pop_front();
	// the fourth argument is not needed ?
	if (loader != nullptr) {
		std::wcerr << "Now don't support java/lang/Class::forName()'s argument `loader` is Application loader!! only support BootStrapLoader!!" << std::endl;
		assert(false);
	} else {
		std::wcout << klass_name << std::endl;
		shared_ptr<Klass> klass = BootStrapClassLoader::get_bootstrap().loadClass(klass_name + L".class");
		assert(klass != nullptr);		// wrong. Because user want to load a non-exist class.
		// because my BootStrapLoader inner doesn't has BasicType Klass. So we don't need to judge whether it's a BasicTypeKlass.
		if (initialize) {
			if (klass->get_type() == ClassType::InstanceClass)	// not an ArrayKlass
				BytecodeEngine::initial_clinit(std::static_pointer_cast<InstanceKlass>(klass), vm);
		}
		_stack.push_back(klass->get_mirror());
	}
}
void JVM_GetSuperClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassInterfaces(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassLoader(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsInterface(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsInstance(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsAssignableFrom(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassSigners(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_SetClassSigners(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsArrayClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_IsPrimitiveClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetComponentType(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassModifiers(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassDeclaredFields(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassDeclaredMethods(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassDeclaredConstructors(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetProtectionDomain(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetDeclaredClasses(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetDeclaringClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassSignature(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassAnnotations(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassConstantPool(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_DesiredAssertionStatus(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	// TODO: 我默认禁止了断言！！ assert 是没有用的。默认是不加 -ea （逃
	// TODO: 关于 assert 字节码的生成，还没有搞清楚。搞清楚了之后立马加上。也可以参见 hotspot: vm/prims/jvm.cpp:2230 --> JVM_DesiredAssertionStatus.
	_stack.push_back(new BooleanOop(false));
}
void JVM_GetEnclosingMethodInfo(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetClassTypeAnnotations(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}
void JVM_GetPrimitiveClass(list<Oop *> & _stack){		// static
	wstring basic_type_klass_name = java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.front());	_stack.pop_front();
#ifdef DEBUG
	std::wcout << "(DEBUG) get BasicTypeMirror of `" << basic_type_klass_name << "`" << std::endl;
#endif
	auto get_basic_type_mirror = [](const wstring & name) -> MirrorOop * {
		auto basic_type_mirror_iter = java_lang_class::get_single_basic_type_mirrors().find(name);
		assert(basic_type_mirror_iter != java_lang_class::get_single_basic_type_mirrors().end());
		return basic_type_mirror_iter->second;
	};
	if (basic_type_klass_name == L"byte") {
		_stack.push_back(get_basic_type_mirror(L"B"));
	} else if (basic_type_klass_name == L"boolean") {
		_stack.push_back(get_basic_type_mirror(L"Z"));
	} else if (basic_type_klass_name == L"char") {
		_stack.push_back(get_basic_type_mirror(L"C"));
	} else if (basic_type_klass_name == L"short") {
		_stack.push_back(get_basic_type_mirror(L"S"));
	} else if (basic_type_klass_name == L"int") {
		_stack.push_back(get_basic_type_mirror(L"I"));
	} else if (basic_type_klass_name == L"float") {
		_stack.push_back(get_basic_type_mirror(L"F"));
	} else if (basic_type_klass_name == L"long") {
		_stack.push_back(get_basic_type_mirror(L"J"));
	} else if (basic_type_klass_name == L"double") {
		_stack.push_back(get_basic_type_mirror(L"D"));
	} else {
		std::wcerr << "can't get here!" << std::endl;
		assert(false);
	}
}

// 返回 fnPtr.
void *java_lang_class_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

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
#include <regex>
#include "utils/synchronize_wcout.hpp"

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
	delay_mirrors.push(L"V");	// void...
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
	auto & delay_mirrors = get_single_delay_mirrors();
	while(!delay_mirrors.empty()) {
		wstring name = delay_mirrors.front();
		delay_mirrors.pop();

		Klass *klass = system_classmap.find(L"java/lang/Class.class")->second;
		if (name.size() == 1)	// ... switch only accept an integer... can't accept a wstring.
			switch (name[0]) {
				case L'I':case L'Z':case L'B':case L'C':case L'S':case L'F':case L'J':case L'D':case L'V':{	// include `void`.
					// insert into.
					MirrorOop *basic_type_mirror = ((MirrorKlass *)klass)->new_mirror(nullptr, nullptr);
					basic_type_mirror->set_extra(name);			// set the name `I`, `J` if it's a primitve type.
					get_single_basic_type_mirrors().insert(make_pair(name, basic_type_mirror));
					break;
				}
				default:{
					assert(false);
				}
			}
		else if (name.size() == 2 && name[0] == L'[') {
			switch (name[1]) {
				case L'I':case L'Z':case L'B':case L'C':case L'S':case L'F':case L'J':case L'D':{
					MirrorOop *basic_type_mirror = ((MirrorKlass *)klass)->new_mirror(nullptr, nullptr);
					get_single_basic_type_mirrors().insert(make_pair(name, basic_type_mirror));
					auto arr_klass = BootStrapClassLoader::get_bootstrap().loadClass(name);		// load the simple array klass first.
					basic_type_mirror->set_mirrored_who(arr_klass);
					arr_klass->set_mirror(basic_type_mirror);
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
			(*iter).second->set_mirror(((MirrorKlass *)klass)->new_mirror(((InstanceKlass *)(*iter).second), nullptr));
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
	return nullptr;
}

void java_lang_class::if_Class_didnt_load_then_delay(Klass *klass, MirrorOop *loader_mirror) {
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
//				java_lang_class::get_single_delay_mirrors().push(((Obj *)) + L".class");
		} else {
			assert(false);
		}
	} else {
		if (klass->get_type() == ClassType::InstanceClass) {
			klass->set_mirror(((MirrorKlass *)klass)->new_mirror((InstanceKlass *)klass, loader_mirror));	// set java_mirror
		}
		else if (klass->get_type() == ClassType::TypeArrayClass) {
			MirrorOop *mirror;
			if ((mirror = get_basic_type_mirror(klass->get_name())) == nullptr) {
				mirror = ((MirrorKlass *)klass)->new_mirror(klass, nullptr);
				get_single_basic_type_mirrors().insert(make_pair(klass->get_name(), mirror));
			}
			klass->set_mirror(mirror);	// set java_mirror
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) add array " << klass->get_name() << "'s mirror..." << std::endl;
#endif
		} else if (klass->get_type() == ClassType::ObjArrayClass) {
			MirrorOop *mirror = ((MirrorKlass *)klass)->new_mirror(klass, nullptr);
			klass->set_mirror(mirror);	// set java_mirror
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) add array " << klass->get_name() << "'s mirror..." << std::endl;
#endif
		} else {
			assert(false);
		}
	}
}

/*===----------------------- Natives ----------------------------===*/
static unordered_map<wstring, void*> methods = {
    {L"getName0:()" STR,						(void *)&JVM_GetClassName},
    {L"forName0:(" STR L"Z" JCL CLS ")" CLS,	(void *)&JVM_ForClassName},
    {L"getSuperclass:()" CLS,				(void *)&JVM_GetSuperClass},
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

void JVM_GetClassName(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(_this != nullptr);
	if (_this->get_mirrored_who() == nullptr) {		// primitive type
		assert(_this->get_extra().size() == 1);
		wchar_t type = _this->get_extra()[0];
		switch (type) {
			case L'B':
				_stack.push_back(java_lang_string::intern(L"byte"));
				break;
			case L'Z':
				_stack.push_back(java_lang_string::intern(L"boolean"));
				break;
			case L'S':
				_stack.push_back(java_lang_string::intern(L"short"));
				break;
			case L'C':
				_stack.push_back(java_lang_string::intern(L"char"));
				break;
			case L'I':
				_stack.push_back(java_lang_string::intern(L"int"));
				break;
			case L'F':
				_stack.push_back(java_lang_string::intern(L"float"));
				break;
			case L'J':
				_stack.push_back(java_lang_string::intern(L"long"));
				break;
			case L'D':
				_stack.push_back(java_lang_string::intern(L"double"));
				break;
			case L'V':
				_stack.push_back(java_lang_string::intern(L"void"));
				break;
			default:
				assert(false);
		}
	} else {
		Oop *str = java_lang_string::intern(std::regex_replace(_this->get_mirrored_who()->get_name(), std::wregex(L"/"), L"."));
		_stack.push_back(str);
	}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) native method [java/lang/Class.getName0()] get `this` classname: [" << java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.back()) << "]." << std::endl;
#endif
}

void JVM_ForClassName(list<Oop *> & _stack){		// static
	vm_thread & thread = *(vm_thread *)_stack.back();	_stack.pop_back();
	wstring klass_name = java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.front());	_stack.pop_front();
//	bool initialize = ((BooleanOop *)_stack.front())->value;	_stack.pop_front();
	bool initialize = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	InstanceOop *loader = (InstanceOop *)_stack.front();	_stack.pop_front();
	// the fourth argument is not needed ?
	if (loader != nullptr) {
		std::wcerr << "Now don't support java/lang/Class::forName()'s argument `loader` is Application loader!! only support BootStrapLoader!!" << std::endl;
		assert(false);
	} else {
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) forClassName: [" << klass_name << "]." << std::endl;
#endif
		Klass *klass = BootStrapClassLoader::get_bootstrap().loadClass(std::regex_replace(klass_name, std::wregex(L"\\."), L"/"));
		assert(klass != nullptr);		// wrong. Because user want to load a non-exist class.
		// because my BootStrapLoader inner doesn't has BasicType Klass. So we don't need to judge whether it's a BasicTypeKlass.
		if (initialize) {
			if (klass->get_type() == ClassType::InstanceClass)	// not an ArrayKlass
				BytecodeEngine::initial_clinit(((InstanceKlass *)klass), thread);
		}
		_stack.push_back(klass->get_mirror());
	}
}

void JVM_GetSuperClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();

	assert(_this != nullptr);

	if (_this->get_mirrored_who() == nullptr) {	// primitive types
		_stack.push_back(nullptr);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) primitive type [" << _this->get_extra() << "] doesn't have a super klass. return null." << std::endl;
#endif
	} else {
		if (_this->get_mirrored_who()->get_parent() == nullptr) {
			assert(_this->get_mirrored_who()->get_name() == L"java/lang/Object");
			_stack.push_back(nullptr);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) java/lang/Object doesn't have a super klass. return null." << std::endl;
#endif
		} else {
			_stack.push_back(_this->get_mirrored_who()->get_parent()->get_mirror());
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) klass type [" << _this->get_mirrored_who()->get_name() << "] have a super klass: [" << _this->get_mirrored_who()->get_parent()->get_name() << "]. return it~" << std::endl;
#endif
		}
	}
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
	assert(_this != nullptr);
	if (_this->get_mirrored_who()) {	// not primitive class
		auto klass = _this->get_mirrored_who();
		if (klass->get_type() == ClassType::InstanceClass) {
			_stack.push_back(new IntOop(((InstanceKlass *)klass)->is_interface()));
		} else {
			_stack.push_back(new IntOop(false));
		}
	} else {
		_stack.push_back(new IntOop(false));
	}
#ifdef DEBUG
	wstring name = _this->get_mirrored_who() == nullptr ? _this->get_extra() : _this->get_mirrored_who()->get_name();
	sync_wcout{} << "(DEBUG) this klass: [" << name << "] is an interface ? [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
}

void JVM_IsInstance(list<Oop *> & _stack){		// is obj a `this` klass's instance?
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	InstanceOop *obj = (InstanceOop *)_stack.front();	_stack.pop_front();

	assert(obj != nullptr);
	assert(_this->get_mirrored_who() != nullptr);

	auto obj_klass = ((InstanceKlass *)obj->get_klass());
	auto this_klass = ((InstanceKlass *)_this->get_mirrored_who());

	if (obj_klass == this_klass || obj_klass->check_parent(this_klass) || obj_klass->check_interfaces(this_klass))
		_stack.push_back(new IntOop(true));
	else
		_stack.push_back(new IntOop(false));

#ifdef DEBUG
	sync_wcout{} << "is [" << obj_klass->get_name() << "]'s oop is also [" << this_klass->get_name() << "]'s instance? [" << std::boolalpha << (bool)(((IntOop *)_stack.back())->value) << "]." << std::endl;
#endif
}

void JVM_IsAssignableFrom(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	MirrorOop *_that = (MirrorOop *)_stack.front();	_stack.pop_front();

	assert(_this != nullptr);
	assert(_that != nullptr);

	if (_this->get_mirrored_who() == nullptr || _that->get_mirrored_who() == nullptr) {	// primitive type
		if (_this == _that)	_stack.push_back(new IntOop(true));
		else					_stack.push_back(new IntOop(false));
#ifdef DEBUG
	sync_wcout{} << "compare with [" << _this->get_extra() << "] and [" << _that->get_extra() << "], result is [" << ((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
	} else {
		// both are not primitive types.
		auto sub = _this->get_klass();
		auto super = _that->get_klass();
#ifdef DEBUG
		sync_wcout{} << "compare with: " << sub->get_name() << " and " << super->get_name() << std::endl;
#endif
		if (sub->get_type() == ClassType::InstanceClass && sub->get_type() == ClassType::InstanceClass) {
			auto real_sub = ((InstanceKlass *)sub);
			auto real_super = ((InstanceKlass *)super);
			if (real_sub == real_super) {
				_stack.push_back(new IntOop(true));
			} else if (real_sub->check_interfaces(real_super) || real_sub->check_parent(real_super)) {
				_stack.push_back(new IntOop(true));
			} else {
				_stack.push_back(new IntOop(false));
			}
#ifdef DEBUG
	sync_wcout{} << "compare with [" << real_sub->get_name() << "] and [" << real_super->get_name() << "], result is [" << ((IntOop *)_stack.back())->value << "]." << std::endl;
#endif
		} else {
			std::wcerr << "I don't know how about ArrayKlass here..." << std::endl;
			assert(false);
		}
	}

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
	auto klass = _this->get_mirrored_who();
	if (klass == nullptr) {		// it is primitive type. klass->get_type will crash.
		_stack.push_back(new IntOop(false));
	} else if (klass->get_type() == ClassType::ObjArrayClass || klass->get_type() == ClassType::TypeArrayClass) {
		_stack.push_back(new IntOop(true));
	} else {
		_stack.push_back(new IntOop(false));
	}
#ifdef DEBUG
	if (klass != nullptr)
		sync_wcout{} << "(DEBUG) klass: [" << klass->get_name() << "] is a array klass? [" << std::boolalpha << (bool)((IntOop *)_stack.back())->value << "]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) klass: [" << _this->get_extra() << "] is not a array klass." << std::endl;
#endif
}

void JVM_IsPrimitiveClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	if (_this->get_mirrored_who()) {
		assert(_this->get_extra() == L"");
		_stack.push_back(new IntOop(false));
#ifdef DEBUG
	sync_wcout{} << "[" << _this->get_mirrored_who()->get_name() << "] is not a Primitive klass. return false." << std::endl;
#endif
	} else {
		_stack.push_back(new IntOop(true));
#ifdef DEBUG
	sync_wcout{} << "[" << _this->get_extra() << "] is a Primitive klass. return true." << std::endl;
#endif
	}
}

void JVM_GetComponentType(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	auto klass = _this->get_mirrored_who();
	assert(klass != nullptr);
	assert(klass->get_type() == ClassType::ObjArrayClass || klass->get_type() == ClassType::TypeArrayClass);

	if (klass->get_type() == ClassType::ObjArrayClass) {
		_stack.push_back(((ObjArrayKlass *)klass)->get_element_klass()->get_mirror());
	} else {
		Type type = ((TypeArrayKlass *)klass)->get_basic_type();
		switch (type) {
			case Type::BYTE:
			case Type::BOOLEAN:
			case Type::SHORT:
			case Type::CHAR:
			case Type::INT:
				_stack.push_back(java_lang_class::get_basic_type_mirror(L"I"));
				break;
			case Type::FLOAT:
				_stack.push_back(java_lang_class::get_basic_type_mirror(L"F"));
				break;
			case Type::LONG:
				_stack.push_back(java_lang_class::get_basic_type_mirror(L"J"));
				break;
			case Type::DOUBLE:
				_stack.push_back(java_lang_class::get_basic_type_mirror(L"D"));
				break;
			default:{
				std::cerr << "can't get here!" << std::endl;
				assert(false);
			}
		}
	}

#ifdef DEBUG
	MirrorOop *mirror = (MirrorOop *)_stack.back();
	if (mirror->get_mirrored_who() == nullptr) {
		sync_wcout{} << "(DEBUG) the element type of ArrayKlass [" << klass->get_name() << "] is [" << mirror->get_extra() << "]." << std::endl;
	} else {
		sync_wcout{} << "(DEBUG) the element type of ArrayKlass [" << klass->get_name() << "] is [" << mirror->get_mirrored_who()->get_name() << "]." << std::endl;
	}
#endif
}

void JVM_GetClassModifiers(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();

	if (_this->get_mirrored_who() == nullptr) {	// primitive types		// see openjdk.
		_stack.push_back(new IntOop(ACC_ABSTRACT | ACC_FINAL | ACC_PUBLIC));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) primitive type [" << _this->get_extra() << "]'s modifier is " << ((IntOop *)_stack.back())->value << std::endl;
#endif
	} else {
		_stack.push_back(new IntOop(_this->get_mirrored_who()->get_access_flags()));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) klass type [" << _this->get_mirrored_who()->get_name() << "]'s modifier is " << ((IntOop *)_stack.back())->value << std::endl;
#endif
	}

}

void JVM_GetClassDeclaredFields(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	bool public_only = (bool)((IntOop *)_stack.front())->value;	_stack.pop_front();


	// check
	auto klass = _this->get_mirrored_who();
	assert(klass->get_type() == ClassType::InstanceClass);

	// get all fields-layout from the mirror's `mirrored_who`.
	auto all_fields = _this->get_mirrored_all_fields();			// make a copy...
	auto & all_static_fields = _this->get_mirrored_all_static_fields();
	for (auto iter : all_static_fields) {
		assert(all_fields.find(iter.first) == all_fields.end());
		all_fields[iter.first] = iter.second;		// fill in.
	}

	// load java/lang/reflect/Field and [Ljava/lang/reflect/Field;.
	auto Field_klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/reflect/Field"));
	assert(Field_klass != nullptr);
	auto Field_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/reflect/Field;"));
	assert(Field_arr_klass != nullptr);
	auto Byte_arr_klass = ((TypeArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[B"));
	assert(Byte_arr_klass != nullptr);

	// create [Java] java/lang/reflect/Field[] from [C++] Field_info *.
	vector<InstanceOop *> v;		// temp save, because we don't know the Field[]'s length now. We should make a traverse.
	for (const auto & iter : all_fields) {
		Field_info *field = iter.second.second;
		if (field->is_static() || _this->is_the_field_owned_by_this(iter.second.first)) {
			if (public_only && !field->is_public()) continue;

			// create a Field oop obj.
			auto field_oop = Field_klass->new_instance();

			// fill in!		// see: openjdk: share/vm/runtime/reflection.cpp
			field_oop->set_field_value(FIELD L":clazz:Ljava/lang/Class;", field->get_klass()->get_mirror());
			field_oop->set_field_value(FIELD L":slot:I", new IntOop(iter.second.first));
			field_oop->set_field_value(FIELD L":name:Ljava/lang/String;", java_lang_string::intern(field->get_name()));

			// judge whether it is a basic type?
			if (field->get_type_klass() != nullptr) {		// It is an obj/objArray/TypeArray.
				field_oop->set_field_value(FIELD L":type:Ljava/lang/Class;", field->get_type_klass()->get_mirror());
			} else {										// It is a BasicType.
				wstring descriptor = field->get_descriptor();
				if (descriptor.size() == 1) {		// BasicType, like `I`, `J`, `S`...
					switch(descriptor[0]) {
						case L'Z':
						case L'B':
						case L'C':
						case L'S':
						case L'I':
						case L'F':
						case L'J':
						case L'D':{
							MirrorOop *basic_mirror = java_lang_class::get_basic_type_mirror(descriptor);
							assert(basic_mirror != nullptr);
							field_oop->set_field_value(FIELD L":type:Ljava/lang/Class;", basic_mirror);
							break;
						}
						default:
							assert(false);
					}
				}
				else assert(false);
			}
			field_oop->set_field_value(FIELD L":modifiers:I", new IntOop(field->get_flag() & (~ACC_ANNOTATION)));
			field_oop->set_field_value(L"java/lang/reflect/AccessibleObject:override:Z", new IntOop(false));
			// set Generic Signature.
			wstring template_signature = field->parse_signature();
			if (template_signature != L"")
				field_oop->set_field_value(FIELD L":signature:Ljava/lang/String;", java_lang_string::intern(template_signature));	// TODO: transient...???
			// set Annotation...
			CodeStub *stub = field->get_rva();		// RuntimeVisibleAnnotations' bytecode
			if (stub) {
				ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
				for (int i = 0; i < stub->stub.size(); i ++) {
					(*byte_arr)[i] = new IntOop(stub->stub[i]);
				}
				field_oop->set_field_value(FIELD L":annotations:[B", byte_arr);
			}

			v.push_back(field_oop);
		}
	}

	ArrayOop *field_arr = Field_arr_klass->new_instance(v.size());
	for (int i = 0; i < v.size(); i ++) {
		(*field_arr)[i] = v[i];
	}

#ifdef DEBUG
	sync_wcout{} << "===-------------- getClassDeclaredFields Pool (" << klass->get_name() << ")-------------===" << std::endl;
	for (int i = 0; i < v.size(); i ++) {
		Oop *result;
		v[i]->get_field_value(FIELD L":name:Ljava/lang/String;", &result);
		sync_wcout{} << java_lang_string::stringOop_to_wstring((InstanceOop *)result) << ", address: [" << result << ']' << std::endl;
	}
	sync_wcout{} << "===--------------------------------------------------------===" << std::endl;
#endif

	_stack.push_back(field_arr);
}

void JVM_GetClassDeclaredMethods(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	bool public_only = (bool)((IntOop *)_stack.front())->value;	_stack.pop_front();

	assert(_this != nullptr);
	assert(_this->get_mirrored_who()->get_type() == ClassType::InstanceClass);

	// load java/lang/reflect/Method
	auto klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/reflect/Method"));
	assert(klass != nullptr);
	auto Method_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/reflect/Method;"));
	assert(Method_arr_klass != nullptr);
	auto Byte_arr_klass = ((TypeArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[B"));
	assert(Byte_arr_klass != nullptr);

	// get all this_klass_methods except <clinit>.	// see JDK API.
	vector<pair<int, Method *>> methods = ((InstanceKlass *)_this->get_mirrored_who())->get_declared_methods();

	vector<InstanceOop *> v;

	// become java/lang/reflect/Constructor
	for (auto iter : methods) {
		if (public_only && !iter.second->is_public())		continue;
		Method *method = iter.second;
		auto method_oop = klass->new_instance();

		method_oop->set_field_value(METHOD L":clazz:Ljava/lang/Class;", method->get_klass()->get_mirror());
		method_oop->set_field_value(METHOD L":slot:I", new IntOop(iter.first));
		method_oop->set_field_value(METHOD L":name:Ljava/lang/String;", java_lang_string::intern(method->get_name()));

		// parse return type:
		auto return_mirror = method->parse_return_type();
		method_oop->set_field_value(METHOD L":returnType:Ljava/lang/Class;", return_mirror);

		// parse arg list.
		vector<MirrorOop *> args = method->parse_argument_list();
		// create [Ljava/lang/Class arr obj.
		ObjArrayKlass * class_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/Class;"));
		auto class_array_obj = class_arr_klass->new_instance(args.size());
		for (int i = 0; i < args.size(); i ++) {
			(*class_array_obj)[i] = args[i];
		}
		method_oop->set_field_value(METHOD L":parameterTypes:[Ljava/lang/Class;", class_array_obj);


		// parse exceptions list.
		vector<MirrorOop *> excepts = method->if_didnt_parse_exceptions_then_parse();
		// create [Ljava/lang/Class arr obj.
		auto except_array_obj = class_arr_klass->new_instance(excepts.size());
		for (int i = 0; i < excepts.size(); i ++) {
			(*except_array_obj)[i] = excepts[i];
		}
		method_oop->set_field_value(METHOD L":exceptionTypes:[Ljava/lang/Class;", except_array_obj);

		method_oop->set_field_value(METHOD L":modifiers:I", new IntOop(method->get_flag() & (~ACC_ANNOTATION)));
		method_oop->set_field_value(L"java/lang/reflect/AccessibleObject:override:Z", new IntOop(false));
		wstring template_signature = method->parse_signature();
		if (template_signature != L"")
			method_oop->set_field_value(METHOD L":signature:Ljava/lang/String;", java_lang_string::intern(template_signature));	// TODO: transient...???

		// set RuntimeVisiableAnnotations
		CodeStub *stub = method->get_rva();		// RuntimeVisibleAnnotations' bytecode
		if (stub) {
			ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
			for (int i = 0; i < stub->stub.size(); i ++) {
				(*byte_arr)[i] = new IntOop(stub->stub[i]);
			}
			method_oop->set_field_value(METHOD L":annotations:[B", byte_arr);
		}
		// set RuntimeVisiableParameterAnnotations
		stub = method->get_rvpa();		// RuntimeVisibleParameterAnnotations' bytecode
		if (stub) {
			ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
			for (int i = 0; i < stub->stub.size(); i ++) {
				(*byte_arr)[i] = new IntOop(stub->stub[i]);
			}
			method_oop->set_field_value(METHOD L":parameterAnnotations:[B", byte_arr);
		}
		// set AnnotationDefault
		stub = method->get_ad();		// AnnotationDefault' bytecode
		if (stub) {
			ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
			for (int i = 0; i < stub->stub.size(); i ++) {
				(*byte_arr)[i] = new IntOop(stub->stub[i]);
			}
			method_oop->set_field_value(METHOD L":annotationDefault:[B", byte_arr);
		}

		v.push_back(method_oop);
	}

	ArrayOop *method_arr = Method_arr_klass->new_instance(v.size());
	for (int i = 0; i < v.size(); i ++) {
		(*method_arr)[i] = v[i];
	}

#ifdef DEBUG
	sync_wcout{} << "===-------------- getClassDeclaredMethods Pool (" << _this->get_mirrored_who()->get_name() << ")-------------===" << std::endl;
	for (int i = 0; i < methods.size(); i ++) {
		sync_wcout{} << i << ". " << methods[i].second->get_name() << ":" << methods[i].second->get_descriptor() << std::endl;
	}
	sync_wcout{} << "===--------------------------------------------------------===" << std::endl;
#endif

	_stack.push_back(method_arr);
}

void JVM_GetClassDeclaredConstructors(list<Oop *> & _stack){

	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	bool public_only = (bool)((IntOop *)_stack.front())->value;	_stack.pop_front();

	assert(_this != nullptr);
	assert(_this->get_mirrored_who()->get_type() == ClassType::InstanceClass);

	// load java/lang/reflect/Constructor
	auto klass = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/reflect/Constructor"));
	assert(klass != nullptr);
	auto Ctor_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/reflect/Constructor;"));
	assert(Ctor_arr_klass != nullptr);
	auto Byte_arr_klass = ((TypeArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[B"));
	assert(Byte_arr_klass != nullptr);

	// get all ctors
	vector<pair<int, Method *>> ctors = ((InstanceKlass *)_this->get_mirrored_who())->get_constructors();

	vector<InstanceOop *> v;

	// become java/lang/reflect/Constructor
	for (auto iter : ctors) {
		if (public_only && !iter.second->is_public())		continue;
		Method *method = iter.second;
		auto ctor_oop = klass->new_instance();

		ctor_oop->set_field_value(CONSTRUCTOR L":clazz:Ljava/lang/Class;", method->get_klass()->get_mirror());
		ctor_oop->set_field_value(CONSTRUCTOR L":slot:I", new IntOop(iter.first));

		// parse arg list.
		vector<MirrorOop *> args = method->parse_argument_list();
		// create [Ljava/lang/Class arr obj.
		ObjArrayKlass * class_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/Class;"));
		auto class_array_obj = class_arr_klass->new_instance(args.size());
		for (int i = 0; i < args.size(); i ++) {
			(*class_array_obj)[i] = args[i];
		}
		ctor_oop->set_field_value(CONSTRUCTOR L":parameterTypes:[Ljava/lang/Class;", class_array_obj);


		// parse exceptions list.
		vector<MirrorOop *> excepts = method->if_didnt_parse_exceptions_then_parse();
		// create [Ljava/lang/Class arr obj.
		auto except_array_obj = class_arr_klass->new_instance(excepts.size());
		for (int i = 0; i < excepts.size(); i ++) {
			(*except_array_obj)[i] = excepts[i];
		}
		ctor_oop->set_field_value(CONSTRUCTOR L":exceptionTypes:[Ljava/lang/Class;", except_array_obj);

		ctor_oop->set_field_value(CONSTRUCTOR L":modifiers:I", new IntOop(method->get_flag() & (~ACC_ANNOTATION)));
		ctor_oop->set_field_value(L"java/lang/reflect/AccessibleObject:override:Z", new IntOop(false));
		wstring template_signature = method->parse_signature();
		if (template_signature != L"")
			ctor_oop->set_field_value(CONSTRUCTOR L":signature:Ljava/lang/String;", java_lang_string::intern(template_signature));	// TODO: transient...???

		// set RuntimeVisiableAnnotations
		CodeStub *stub = method->get_rva();		// RuntimeVisibleAnnotations' bytecode
		if (stub) {
			ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
			for (int i = 0; i < stub->stub.size(); i ++) {
				(*byte_arr)[i] = new IntOop(stub->stub[i]);
			}
			ctor_oop->set_field_value(CONSTRUCTOR L":annotations:[B", byte_arr);
		}
		// set RuntimeVisiableParameterAnnotations
		stub = method->get_rvpa();		// RuntimeVisibleParameterAnnotations' bytecode
		if (stub) {
			ArrayOop *byte_arr = Byte_arr_klass->new_instance(stub->stub.size());
			for (int i = 0; i < stub->stub.size(); i ++) {
				(*byte_arr)[i] = new IntOop(stub->stub[i]);
			}
			ctor_oop->set_field_value(CONSTRUCTOR L":parameterAnnotations:[B", byte_arr);
		}

		v.push_back(ctor_oop);
	}

	ArrayOop *ctor_arr = Ctor_arr_klass->new_instance(v.size());
	for (int i = 0; i < v.size(); i ++) {
		(*ctor_arr)[i] = v[i];
	}

#ifdef DEBUG
	sync_wcout{} << "===-------------- getClassDeclaredCtors Pool (" << _this->get_mirrored_who()->get_name() << ")-------------===" << std::endl;
	for (int i = 0; i < ctors.size(); i ++) {
		sync_wcout{} << i << ". " << ctors[i].second->get_name() << ":" << ctors[i].second->get_descriptor() << std::endl;
	}
	sync_wcout{} << "===--------------------------------------------------------===" << std::endl;
#endif

	_stack.push_back(ctor_arr);
}

void JVM_GetProtectionDomain(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	vm_thread *thread = (vm_thread *)_stack.back();	_stack.pop_back();
//	thread->get_stack_trace();

	_stack.push_back(nullptr);
}

void JVM_GetDeclaredClasses(list<Oop *> & _stack){		// return the mirror's inner: public/protected/private inner classes.
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

void JVM_GetDeclaringClass(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	if (_this == nullptr) {
		_stack.push_back(nullptr);
		return;
	}
	if (_this->get_mirrored_who() == nullptr) {
		_stack.push_back(nullptr);
		return;
	}
	if (_this->get_mirrored_who()->get_type() != ClassType::InstanceClass) {
		_stack.push_back(nullptr);
		return;
	}
	auto klass = ((InstanceKlass *)_this->get_mirrored_who());
	auto rt_pool = klass->get_rtpool();
	auto inner_class_attr = klass->get_inner_class();
	if (inner_class_attr == nullptr) {
		_stack.push_back(nullptr);
		return;
	}

	// traverse
	for (int i = 0; i < inner_class_attr->number_of_classes; i ++) {
		int ioff = inner_class_attr->classes[i].inner_class_info_index;
		int ooff = inner_class_attr->classes[i].outer_class_info_index;
		int noff = inner_class_attr->classes[i].inner_name_index;		// of no use.

		if (ioff != 0) {
			auto target_inner_klass = boost::any_cast<Klass *>((*rt_pool)[ioff-1].second);
			if (target_inner_klass == klass) {		// get the inner is `this`. then find the outer.
				if (ooff == 0) {
					_stack.push_back(nullptr);
					return;
				} else {
					auto target_outer_klass = boost::any_cast<Klass *>((*rt_pool)[ooff-1].second);
					_stack.push_back(target_outer_klass->get_mirror());
					return;
				}
			}
		}
	}

	_stack.push_back(nullptr);
	return;

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
	_stack.push_back(new IntOop(false));
}

void JVM_GetEnclosingMethodInfo(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	if (_this == nullptr) {
		_stack.push_back(nullptr);
		return;
	}
	if (_this->get_mirrored_who() == nullptr) {		// primitive type, wrong.
		_stack.push_back(nullptr);
		return;
	}
	if (_this->get_mirrored_who()->get_type() != ClassType::InstanceClass) {
		_stack.push_back(nullptr);
		return;
	}
	auto klass = ((InstanceKlass *)_this->get_mirrored_who());
	auto enclosing_method_attr = klass->get_enclosing_method();
	if (enclosing_method_attr == nullptr) {
		_stack.push_back(nullptr);
		return;
	}
	assert(enclosing_method_attr->class_index != 0);
	auto rt_pool = klass->get_rtpool();

	// 0. initialize the container: Object[3].
	auto obj_arr_klass = ((ObjArrayKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"[Ljava/lang/Object;"));
	assert(obj_arr_klass != nullptr);
	auto obj_arr = obj_arr_klass->new_instance(3);

	// 1. get the klass: ---> ClassInfo
	int target_klass_index = enclosing_method_attr->class_index;
	auto target_klass = boost::any_cast<Klass *>((*rt_pool)[target_klass_index-1].second);
	assert(target_klass != nullptr);
	// 1.5. put in.
	(*obj_arr)[0] = target_klass->get_mirror();

	// 2. get the method: ---> NameAndTypeInfo
	int target_name_and_type_index = enclosing_method_attr->method_index;
	if (target_name_and_type_index == 0) {	// if it's 0, return.
		_stack.push_back(obj_arr);
		return;
	}
	auto _pair = boost::any_cast<pair<int, int>>((*rt_pool)[target_name_and_type_index-1].second);
	// 2.3. get the String: name
	auto name = java_lang_string::intern(boost::any_cast<wstring>((*rt_pool)[_pair.first-1].second));
	// 2.5. put it in.
	(*obj_arr)[1] = name;
	// 2.6. get the String: descriptor
	auto descriptor = java_lang_string::intern(boost::any_cast<wstring>((*rt_pool)[_pair.second-1].second));
	// 2.8. put it in.
	(*obj_arr)[2] = descriptor;

	// 3.0. return.
	_stack.push_back(obj_arr);

}

void JVM_GetClassTypeAnnotations(list<Oop *> & _stack){
	MirrorOop *_this = (MirrorOop *)_stack.front();	_stack.pop_front();
	assert(false);
}

void JVM_GetPrimitiveClass(list<Oop *> & _stack){		// static
	wstring basic_type_klass_name = java_lang_string::stringOop_to_wstring((InstanceOop *)_stack.front());	_stack.pop_front();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get BasicTypeMirror of `" << basic_type_klass_name << "`" << std::endl;
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
	} else if (basic_type_klass_name == L"void") {		// **IMPORTANT** java/lang/Void!!
		_stack.push_back(get_basic_type_mirror(L"V"));
//		_stack.push_back(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Void")->get_mirror());		// wrong.
	} else {
		std::wcerr << "can't get here!" << std::endl;
		assert(false);
	}
}

void *java_lang_class_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

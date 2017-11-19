/*
 * java_lang_class.cpp
 *
 *  Created on: 2017年11月16日
 *      Author: zhengxiaolin
 */

#include "runtime/java_lang_class.hpp"
#include "runtime/klass.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"

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
					get_single_basic_type_mirrors().insert(make_pair(name, std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(nullptr)));
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
			(*iter).second->set_mirror(std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(std::static_pointer_cast<InstanceKlass>((*iter).second)));
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

void java_lang_class::if_Class_didnt_load_then_delay(shared_ptr<Klass> klass) {
	// set java_mirror
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
		if (klass->get_type() == ClassType::InstanceClass)
			klass->set_mirror(std::static_pointer_cast<MirrorKlass>(klass)->new_mirror(std::static_pointer_cast<InstanceKlass>(klass)));	// set java_mirror
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

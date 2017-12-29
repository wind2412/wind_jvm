/*
 * string_table.cpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_String.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

// hash func
size_t java_string_hash::operator()(Oop* const & ptr) const noexcept
{
	// if has a hash_val cache, need no calculate.
	Oop *int_oop_hash;
	if (((InstanceOop *)ptr)->get_field_value(STRING L":hash:I", &int_oop_hash) == true && ((IntOop *)int_oop_hash)->value != 0) {	// cashed_hash will become a ... IntOop...
		return ((IntOop *)int_oop_hash)->value;
	}

	// get string oop's `value` field's `TypeArrayOop` and calculate hash value	// using **Openjdk8 string hash algorithm!!**
	Oop *value_field;
	((InstanceOop *)ptr)->get_field_value(STRING L":value:[C", &value_field);
	int length = ((TypeArrayOop *)value_field)->get_length();
	int hash_val = 0;
	for (int i = 0; i < length; i ++) {
		hash_val =  31 * hash_val + ((IntOop *)(*((TypeArrayOop *)value_field))[i])->value;
	}
	// make a hashvalue cache
	((InstanceOop *)ptr)->set_field_value(STRING L":hash:I", new IntOop(hash_val));

	return hash_val;
}

// equalor
bool java_string_equal_to::operator() (Oop* const & lhs, Oop* const & rhs) const
{
	if (lhs == rhs)	return true;		// prevent from alias ptr...

	// get `value` field's `char[]` and compare every char.
	Oop *value_field_lhs;
	((InstanceOop *)lhs)->get_field_value(STRING L":value:[C", &value_field_lhs);
	Oop *value_field_rhs;
	((InstanceOop *)rhs)->get_field_value(STRING L":value:[C", &value_field_rhs);

	int length_lhs = ((TypeArrayOop *)value_field_lhs)->get_length();
	int length_rhs = ((TypeArrayOop *)value_field_rhs)->get_length();
	if (length_lhs != length_rhs)	return false;
	for (int i = 0; i < length_lhs; i ++) {
		if ( ((IntOop *)(*((TypeArrayOop *)value_field_lhs))[i])->value != ((IntOop *)(*((TypeArrayOop *)value_field_rhs))[i])->value) {
			return false;
		}
	}
	return true;
}


/*===---------------- java_lang_string ----------------===*/
wstring java_lang_string::stringOop_to_wstring(InstanceOop *stringoop) {
	wstringstream ss;
	Oop *result;
	bool temp = stringoop->get_field_value(STRING L":value:[C", &result);
	assert(temp == true);
	// get string literal
	if (result == nullptr) {
		return L"";
	}
	for (int pos = 0; pos < ((TypeArrayOop *)result)->get_length(); pos ++) {
		ss << (wchar_t)((IntOop *)(*(TypeArrayOop *)result)[pos])->value;
	}
	return ss.str();
}

wstring java_lang_string::print_stringOop(InstanceOop *stringoop) {
	wstringstream ss;
	Oop *result;
	bool temp = stringoop->get_field_value(STRING L":value:[C", &result);
	assert(temp == true);
	// get length
	ss << "string length: [" << ((TypeArrayOop *)result)->get_length() << "] ";
	// get string literal
	ss << "string is: --> [\"";
	if (result != nullptr)		// bug report, 见上！
		for (int pos = 0; pos < ((TypeArrayOop *)result)->get_length(); pos ++) {
			ss << (wchar_t)((IntOop *)(*(TypeArrayOop *)result)[pos])->value;
		}
	ss << "\"]";
	// get hash value
	Oop *int_oop_hash;
	assert(stringoop->get_field_value(STRING L":hash:I", &int_oop_hash) == true);
	ss << " hash is: [" << ((IntOop *)int_oop_hash)->value << "]";
	ss << " address is: [" << stringoop << "]";
	return ss.str();
}

Oop *java_lang_string::intern_to_oop(const wstring & str) {
	assert(system_classmap.find(L"[C.class") != system_classmap.end());
	// alloc a `char[]` for `value` field
	TypeArrayOop * charsequence = (TypeArrayOop *)((TypeArrayKlass *)(*system_classmap.find(L"[C.class")).second)->new_instance(str.size());
	assert(charsequence->get_klass() != nullptr);
	// fill in `char[]`
	for (int pos = 0; pos < str.size(); pos ++) {
		(*charsequence)[pos] = new IntOop((unsigned short)str[pos]);
	}
	// alloc a StringOop.
	InstanceOop *stringoop = ((InstanceKlass *)BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/String"))->new_instance();
	assert(stringoop != nullptr);
	Oop *int_oop_hash;
	assert(stringoop->get_field_value(STRING L":hash:I", &int_oop_hash) == true);
	assert(((IntOop *)int_oop_hash)->value == 0);	// uninitialized.
	stringoop->set_field_value(STRING L":value:[C", charsequence);
	return stringoop;
}


/*===----------------------- Native -----------------------------===*/

static unordered_map<wstring, void*> methods = {
    {L"intern:()" STR,				(void *)&JVM_Intern},
};

void JVM_Intern(list<Oop *> & _stack){		// static
	// this is a java.lang.String alloc on the heap.
	InstanceOop *_this = (InstanceOop *)_stack.front();	_stack.pop_front();		// this string oop begin Interned.
	// now use java_lang_string::intern to alloc it on the StringTable.
	InstanceOop *rt_pool_str = (InstanceOop *)java_lang_string::intern(java_lang_string::stringOop_to_wstring(_this));

	_stack.push_back(rt_pool_str);
}



void *java_lang_string_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}

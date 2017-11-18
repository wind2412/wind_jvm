/*
 * string_table.cpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#include <runtime/java_lang_string.hpp>
#include "runtime/oop.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"

// hash func
size_t java_string_hash::operator()(Oop* ptr) noexcept
{
	// if has a hash_val cache, need no calculate.
	Oop *int_oop_hash;
	if (((InstanceOop *)ptr)->get_field_value(L"hash:I", &int_oop_hash) == true) {	// cashed_hash will become a ... IntOop...
		return ((IntOop *)int_oop_hash)->value;
	}

	// get string oop's `value` field's `TypeArrayOop` and calculate hash value	// using **Openjdk8 string hash algorithm!!**
	Oop *value_field;
	assert(((InstanceOop *)ptr)->get_field_value(L"value:[C", &value_field) == true);
	int length = ((TypeArrayOop *)value_field)->get_length();
	unsigned int hash_val = 0;
	for (int i = 0; i < length; i ++) {
		hash_val =  31 * hash_val + ((CharOop *)&((TypeArrayOop *)value_field)[i])->value;
	}
	// 这里需要注意！！由于 java.lang.String 这个对象是被伪造出来，在 openjdk 的实现是：`value` field 被强行注入，但是 `hashcode` field 被惰性算出。这里算出之后会直接 save 到 oop 中！
	// make a hashvalue cache
	((InstanceOop *)ptr)->set_field_value(L"hash:I", new IntOop(hash_val));

	return hash_val;
}

// equalor
bool java_string_equal_to::operator() (const Oop *lhs, const Oop *rhs) const
{
	if (lhs == rhs)	return true;		// prevent from alias ptr...

	// get `value` field's `char[]` and compare every char.
	Oop *value_field_lhs;
	assert(((InstanceOop *)lhs)->get_field_value(L"value:[C", &value_field_lhs) == true);
	Oop *value_field_rhs;
	assert(((InstanceOop *)rhs)->get_field_value(L"value:[C", &value_field_rhs) == true);

	int length_lhs = ((TypeArrayOop *)value_field_lhs)->get_length();
	int length_rhs = ((TypeArrayOop *)value_field_rhs)->get_length();
	if (length_lhs != length_rhs)	return false;
	for (int i = 0; i < length_lhs; i ++) {
		if (((CharOop *)&((TypeArrayOop *)value_field_lhs)[i])->value != ((CharOop *)&((TypeArrayOop *)value_field_rhs)[i])->value) {
			return false;
		}
	}
	return true;
}


/*===---------------- java_lang_string ----------------===*/
Oop *java_lang_string::intern_to_oop(const wstring & str) {
	assert(system_classmap.find(L"[C.class") != system_classmap.end());
	// alloc a `char[]` for `value` field
	TypeArrayOop * charsequence = (TypeArrayOop *)std::static_pointer_cast<TypeArrayKlass>((*system_classmap.find(L"[C.class")).second)->new_instance(str.size());
	assert(charsequence->get_klass() != nullptr);
	// fill in `char[]`
	for (int pos = 0; pos < str.size(); pos ++) {
		(*charsequence)[pos] = new CharOop((uint32_t)str[pos]);
	}
	// alloc a StringOop.
	InstanceOop *stringoop = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/String"))->new_instance();
	assert(stringoop != nullptr);
	stringoop->set_field_value(L"value:[C", charsequence);		// 直接钦定 value 域，并且 encode，可以 decode 为 TypeArrayOop* 。原先设计为 Oop* 全是 shared_ptr<Oop>，不过这样到了这步，引用计数将会不准...因为 shared_ptr 无法变成 uint_64，所以就会使用 shared_ptr::get()。所以去掉了 shared_ptr<Oop>，成为了 Oop *。
#ifdef DEBUG
	Oop *result;
	bool temp = stringoop->get_field_value(L"value:[C", &result);
	assert(result == charsequence);
	assert(temp == true);
	std::cout << "string length: " << ((TypeArrayOop *)result)->get_length() << std::endl;
	std::cout << "the string is: --> ";
	for (int pos = 0; pos < ((TypeArrayOop *)result)->get_length(); pos ++) {
		std::wcout << wchar_t(((CharOop *)(*(TypeArrayOop *)result)[pos])->value);
	}
	std::wcout.clear();
	std::wcout << std::endl;
#endif
	return stringoop;
}


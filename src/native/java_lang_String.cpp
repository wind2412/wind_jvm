/*
 * string_table.cpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_String.hpp"
#include "runtime/oop.hpp"
#include "system_directory.hpp"
#include "classloader.hpp"

// hash func
size_t java_string_hash::operator()(Oop* const & ptr) const noexcept		// TODO: 原先写的是 Oop *，在 g++ 上报错，在 clang++ 上不报错...!!  而且 g++ 规定 const 必须要加......
{
	// if has a hash_val cache, need no calculate.
	Oop *int_oop_hash;
	if (((InstanceOop *)ptr)->get_field_value(L"hash:I", &int_oop_hash) == true && ((IntOop *)int_oop_hash)->value != 0) {	// cashed_hash will become a ... IntOop...
		// 基本不可能是 0.如果是 0，那么就重算就好。如果重算还是 0，那就是 0 了。
		return ((IntOop *)int_oop_hash)->value;
	}

	// get string oop's `value` field's `TypeArrayOop` and calculate hash value	// using **Openjdk8 string hash algorithm!!**
	Oop *value_field;
	assert(((InstanceOop *)ptr)->get_field_value(L"value:[C", &value_field) == true);
	int length = ((TypeArrayOop *)value_field)->get_length();
	unsigned int hash_val = 0;
	for (int i = 0; i < length; i ++) {
		hash_val =  31 * hash_val + ((CharOop *)(*((TypeArrayOop *)value_field))[i])->value;
	}
	// 这里需要注意！！由于 java.lang.String 这个对象是被伪造出来，在 openjdk 的实现是：`value` field 被强行注入，但是 `hashcode` field 被惰性算出。这里算出之后会直接 save 到 oop 中！
	// make a hashvalue cache
	((InstanceOop *)ptr)->set_field_value(L"hash:I", new IntOop(hash_val));

	return hash_val;
}

// equalor
bool java_string_equal_to::operator() (Oop* const & lhs, Oop* const & rhs) const
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
		if ( ((CharOop *)(*((TypeArrayOop *)value_field_lhs))[i])->value !=  ((CharOop *)(*((TypeArrayOop *)value_field_rhs))[i])->value) {
			return false;
		}
	}
	return true;
}


/*===---------------- java_lang_string ----------------===*/
wstring java_lang_string::print_stringOop(InstanceOop *stringoop) {
	wstringstream ss;
	Oop *result;
	bool temp = stringoop->get_field_value(L"value:[C", &result);
	assert(temp == true);
	// get length
	ss << "string length: [" << ((TypeArrayOop *)result)->get_length() << "] ";
	// get string literal
	ss << "string is: --> [\"";
	for (int pos = 0; pos < ((TypeArrayOop *)result)->get_length(); pos ++) {
		ss << wchar_t(((CharOop *)(*(TypeArrayOop *)result)[pos])->value);
	}
	ss << "\"]";
	// get hash value
	Oop *int_oop_hash;
	assert(stringoop->get_field_value(L"hash:I", &int_oop_hash) == true);
	ss << " hash is: [" << ((IntOop *)int_oop_hash)->value << "]";
	return ss.str();
}

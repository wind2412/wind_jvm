/*
 * string_table.hpp
 *
 *  Created on: 2017年11月14日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_JAVA_LANG_STRING_HPP_
#define INCLUDE_RUNTIME_JAVA_LANG_STRING_HPP_

#include <unordered_set>
#include <memory>
#include <string>

using std::wstring;
using std::shared_ptr;
using std::unordered_set;

// this code is from javaClasses.hpp, openjdk8 hotspot src code. T is jchar, (unsigned short).
//template <typename T> static unsigned int hash_code(T* s, int len) {
//    unsigned int h = 0;
//    while (len-- > 0) {
//      h = 31*h + (unsigned int) *s;
//      s++;
//    }
//    return h;
//  }

#include "runtime/oop.hpp"		 // delete
#include "system_directory.hpp"	 // delete
#include "classloader.hpp"		 // delete

struct java_string_hash	// Hash 用于决定此 T 变量放到哪个桶中，equator 才是判断两个 T 相不相同的重要因素！！
{
	size_t operator()(Oop* ptr) noexcept
	{
		// if has a hash_val cache, need no calculate.
		uint64_t int_oop_hash;
		if (((InstanceOop *)ptr)->get_field_value(L"hash:I", &int_oop_hash) == true) {	// cashed_hash will become a ... IntOop...
			return ((IntOop *)int_oop_hash)->value;
		}

		// get string oop's `value` field's `TypeArrayOop` and calculate hash value	// using **Openjdk8 string hash algorithm!!**
		uint64_t value_field;
		assert(((InstanceOop *)ptr)->get_field_value(L"value:[C", &value_field) == true);
		int length = ((TypeArrayOop *)value_field)->get_length();
		unsigned int hash_val = 0;
		for (int i = 0; i < length; i ++) {
			hash_val =  31 * hash_val + ((CharOop *)&((TypeArrayOop *)value_field)[i])->value;
		}
		// 这里需要注意！！由于 java.lang.String 这个对象是被伪造出来，在 openjdk 的实现是：`value` field 被强行注入，但是 `hashcode` field 被惰性算出。这里算出之后会直接 save 到 oop 中！
		// make a hashvalue cache
		((InstanceOop *)ptr)->set_field_value(L"hash:I", (uint64_t)new IntOop(hash_val));

		return hash_val;
	}
	size_t operator()(const wstring & str)
	{
		for (int i = 0; i < str.size(); i ++) {

		}
	}
};

struct java_string_equal_to
{
	bool operator() (const Oop *lhs, const Oop *rhs) const
	{
		if (lhs == rhs)	return true;		// prevent from alias ptr...

		// get `value` field's `char[]` and compare every char.
		uint64_t value_field_lhs;
		assert(((InstanceOop *)lhs)->get_field_value(L"value:[C", &value_field_lhs) == true);
		uint64_t value_field_rhs;
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
};

class java_lang_string {
public:
	static auto & get_string_table() {
		static unordered_set<Oop *, java_string_hash, java_string_equal_to> string_table;
		return string_table;
	}
private:
	static Oop *intern_to_oop(const wstring & str) {
		assert(system_classmap.find(L"[C.class") != system_classmap.end());
		// alloc a `char[]` for `value` field
		TypeArrayOop * charsequence = (TypeArrayOop *)std::static_pointer_cast<TypeArrayKlass>((*system_classmap.find(L"[C.class")).second)->new_instance(str.size());
		assert(charsequence->get_klass() != nullptr);
		// fill in `char[]`
		for (int pos = 0; pos < str.size(); pos ++) {
			(*charsequence)[pos] = new CharOop((uint32_t)str[pos]);
		}
		// alloc a StringOop.
		InstanceOop *stringoop = new InstanceOop(std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/String")));
		assert(stringoop != nullptr);
		stringoop->set_field_value(L"value:[C", (uint64_t)charsequence);		// 直接钦定 value 域，并且 encode，可以 decode 为 TypeArrayOop* 。原先设计为 Oop* 全是 shared_ptr<Oop>，不过这样到了这步，引用计数将会不准...因为 shared_ptr 无法变成 uint_64，所以就会使用 shared_ptr::get()。所以去掉了 shared_ptr<Oop>，成为了 Oop *。
		return stringoop;
	}
public:
	static inline Oop *intern(const wstring & str) {		// intern 用到的次数非常多。建议内联。
		Oop *stringoop = java_lang_string::intern_to_oop(str);							// TODO: 注意！！这里也用了 new，但是没有放到 GC 堆当中............
		auto iter = java_lang_string::get_string_table().find(stringoop);
		if (iter == java_lang_string::get_string_table().end()) {
			java_lang_string::get_string_table().insert(stringoop);
			return stringoop;
		} else {
			return *iter;
		}
	}
};

#endif /* INCLUDE_RUNTIME_JAVA_LANG_STRING_HPP_ */

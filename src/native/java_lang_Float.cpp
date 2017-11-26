/*
 * java_lang_Float.cpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Float.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"floatToRawIntBits:(F)I",				(void *)&JVM_FloatToRawIntBits},
};

// this method, I simplified it to just run the `run()` method.
void JVM_FloatToRawIntBits(list<Oop *> & _stack){		// static
	float v = ((FloatOop *)_stack.front())->value;	_stack.pop_front();

	// openjdk 超级漂亮的解法！！
	// 原先我就想...怎么把 float [按位] 完全转换为 uint64_t 这种来储存，然后这样就可以把所有 BasicType 全都变成 uint64_t 来存放，这样的话由于返回值类型唯一，
	// 类似 boost::any 和 void * 这种完全泛型，就可以只写一个函数就返回这个泛型类型了(由于C系语言不支持一个函数多返回值)。
	// 然而每次 (int)float 肯定有精度丢失......因为会走 C 内部强制转换的算法。所以没想出来，于是就把原来的 BasicTypeOop::get_value 和 set_value 去掉了...
	// 取而代之的是 一大串 switch case...
	// 怎么就没想到 union ！！！（哭
	// 因此完全把 openjdk 的解法搬上来：见下。
	union {
		int i;
		float f;
	} u;
	u.f = (float)v;		// 见 openjdk: share/native/java/lang/Float.c::Java_java_lang_Float_floatToRawIntBits()

#ifdef DEBUG
	std::wcout << "(DEBUG) [use bits] to convert float: [" << u.f << "] to int: [" << u.i << "] and return the int value." << std::endl;
#endif

	_stack.push_back(new IntOop(u.i));
}

// 返回 fnPtr.
void *java_lang_float_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}



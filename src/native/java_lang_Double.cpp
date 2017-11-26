/*
 * java_lang_Double.cpp
 *
 *  Created on: 2017年11月26日
 *      Author: zhengxiaolin
 */

#include "native/java_lang_Double.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"

static unordered_map<wstring, void*> methods = {
    {L"doubleToRawLongBits:(D)J",				(void *)&JVM_DoubleToRawLongBits},
    {L"longBitsToDouble:(J)D",					(void *)&JVM_LongBitsToDouble},
};

// this method, I simplified it to just run the `run()` method.
void JVM_DoubleToRawLongBits(list<Oop *> & _stack){		// static
	double v = ((DoubleOop *)_stack.front())->value;	_stack.pop_front();

	union {
		long l;
		double d;
	} u;
	u.d = v;

#ifdef DEBUG
	std::wcout << "(DEBUG) [use bits] to convert double: [" << u.d << "] to long: [" << u.l << "] and return the long value." << std::endl;
#endif

	_stack.push_back(new LongOop(u.l));
}
void JVM_LongBitsToDouble(list<Oop *> & _stack){		// static
	long v = ((LongOop *)_stack.front())->value;	_stack.pop_front();

	union {
		long l;
		double d;
	} u;
	u.l = v;

#ifdef DEBUG
	std::wcout << "(DEBUG) [use bits] to convert long: [" << u.l << "] to double: [" << u.d << "] and return the double value." << std::endl;
#endif

	_stack.push_back(new DoubleOop(u.d));
}




// 返回 fnPtr.
void *java_lang_double_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}




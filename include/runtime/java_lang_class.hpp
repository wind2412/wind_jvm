/*
 * java_lang_class.hpp
 *
 *  Created on: 2017年11月16日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_
#define INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_

#include <queue>
#include <unordered_map>
#include <string>
#include <cassert>
#include <memory>

using std::queue;
using std::unordered_map;
using std::wstring;
using std::shared_ptr;

class MirrorOop;
class Klass;

class java_lang_class {
public:
	enum mirror_state { UnInited, Inited, Fixed };
public:
	static mirror_state & state();
	static queue<wstring> & get_single_delay_mirrors();
	static unordered_map<wstring, MirrorOop*> & get_single_basic_type_mirrors();
	static void init();		// must execute this method before jvm!!!
	static void fixup_mirrors();	// must execute this after java.lang.Class load!!!
	static MirrorOop *get_basic_type_mirror(const wstring & signature);	// "I", "Z", "D", "J" ......	// must execute this after `fixup_mirrors()` called !!
	static void if_Class_didnt_load_then_delay(shared_ptr<Klass> klass);
};




#endif /* INCLUDE_RUNTIME_JAVA_LANG_CLASS_HPP_ */

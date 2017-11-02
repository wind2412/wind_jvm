#ifndef __META_CLASS_H__
#define __META_CLASS_H__

#include <set>
#include <unordered_map>
#include <string>
#include <class_parser.hpp>
#include <memory>

#include "runtime/field.hpp"
#include "runtime/interface.hpp"
#include "runtime/method.hpp"


using std::wstring;
using std::set;
using std::unordered_map;
using std::shared_ptr;

class ClassFile;

class MetaClass {
public:
	enum State{Zero, Loaded, Parsed, Initialized};
private:
	State cur = Zero;
private:
	shared_ptr<ClassFile> cf;
private:
	wstring name;		// 这个 MetaClass 就设定为运行时 resolve 替换符号引用之后的事情把。
	unordered_map<wstring, Interface> interfaces;
	Field fields;
	Method methods;
public:
	State getState() { return cur; }
	void setState(State s) { cur = s; }
private:
	MetaClass();
	MetaClass(const MetaClass &);
	MetaClass operator= (const MetaClass &);
public:
	static shared_ptr<MetaClass> convert(const ClassFile & cf);
};



#endif

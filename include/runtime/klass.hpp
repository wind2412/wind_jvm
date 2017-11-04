/*
 * klass.hpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_KLASS_HPP_
#define INCLUDE_RUNTIME_KLASS_HPP_

#include "method.hpp"
#include "field.hpp"
#include "constantpool.hpp"
#include <unordered_map>
#include <memory>

using std::unordered_map;
using std::shared_ptr;


/**
 * 这个 Klass 是一定要被 new 出来的！
 */

class Klass {		// similar to java.lang.Class	-->		metaClass	// oopDesc is the real class object's Class.
public:
	enum State{Zero, Loaded, Parsed, Initialized};
protected:
	State cur = Zero;
protected:
	wstring name;		// this class's name		// use constant_pool single string but not copy.
	int access_flags;	// this class's access flags

	Klass *parent;
	Klass *next_sibling;
	Klass *child;
public:
	State get_state() { return cur; }
	void set_state(State s) { cur = s; }
	Klass *get_parent() { return parent; }
	void set_parent(Klass *parent) { this->parent = parent; }
	Klass *get_next_sibling() { return next_sibling; }
	void set_next_sibling(Klass *next_sibling) { this->next_sibling = next_sibling; }
	Klass *get_child() { return child; }
	void set_child(Klass *child) { this->child = child; }
	int get_access_flags() { return access_flags; }
	void set_access_flags(int access_flags) { this->access_flags = access_flags; }
protected:
	Klass(const Klass &);
	Klass operator= (const Klass &);
public:
	Klass() {}
};

class InstanceKlass : public Klass {
private:
	ClassFile cf;		// origin non-dynamic constant pool

	unordered_map<wstring, Klass*> interfaces;
	unordered_map<wstring, Field*> fields;
	unordered_map<wstring, Method*> methods;		// 虽然效率不高，不过为了实现直观起见，使用了基于字符串的查找。

	// 其他的 attributes 稍后再加
private:
	InstanceKlass(const ClassFile & cf, ClassLoader *loader);
};

#endif /* INCLUDE_RUNTIME_KLASS_HPP_ */

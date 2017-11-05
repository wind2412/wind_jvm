/*
 * klass.hpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_RUNTIME_KLASS_HPP_
#define INCLUDE_RUNTIME_KLASS_HPP_

#include "method.hpp"
#include "class_parser.hpp"
#include <unordered_map>
#include <memory>
#include <utility>

using std::unordered_map;
using std::shared_ptr;
using std::pair;

/**
 * 这个 Klass 是一定要被 new 出来的！
 */

class Klass /*: public std::enable_shared_from_this<Klass>*/ {		// similar to java.lang.Class	-->		metaClass	// oopDesc is the real class object's Class.
public:
	enum State{Zero, Loaded, Parsed, Initialized};
protected:
	State cur = Zero;
protected:
	wstring name;		// this class's name		// use constant_pool single string but not copy.
	u2 access_flags;	// this class's access flags

	shared_ptr<Klass> parent;
	shared_ptr<Klass> next_sibling;
	shared_ptr<Klass> child;
public:
	State get_state() { return cur; }
	void set_state(State s) { cur = s; }
	shared_ptr<Klass> get_parent() { return parent; }
	void set_parent(shared_ptr<Klass> parent) { this->parent = parent; }
	shared_ptr<Klass> get_next_sibling() { return next_sibling; }
	void set_next_sibling(shared_ptr<Klass> next_sibling) { this->next_sibling = next_sibling; }
	shared_ptr<Klass> get_child() { return child; }
	void set_child(shared_ptr<Klass> child) { this->child = child; }
	int get_access_flags() { return access_flags; }
	void set_access_flags(int access_flags) { this->access_flags = access_flags; }
	wstring get_name() { return name; }
protected:
	Klass(const Klass &);
	Klass operator= (const Klass &);
public:
	Klass() {}
};

class Fields;
class Field_info;

class InstanceKlass : public Klass {
	friend Fields;
private:
//	ClassFile *cf;		// origin non-dynamic constant pool
	ClassLoader *loader;

	// interfaces
	unordered_map<int, shared_ptr<InstanceKlass>> interfaces;
	// fields (non-static / static)
	unordered_map<int, pair<int, shared_ptr<Field_info>>> fields_layout;			// non-static field layout. [values are in oop].
	unordered_map<int, pair<int, shared_ptr<Field_info>>> static_fields_layout;	// static field layout.	<constant_pool's index, static_fields' offset>
	int total_non_static_fields_bytes = 0;
	int total_static_fields_bytes = 0;
	uint8_t *static_fields;													// static field values. [non-static field values are in oop].
	// methods
	unordered_map<int, Method*> methods;

	// 其他的 attributes 稍后再加
	attribute_info *attributes;
private:
	void parse_methods(const ClassFile & cf);
	void parse_fields(const ClassFile & cf);
	void parse_superclass(const ClassFile & cf, ClassLoader *loader);
	void parse_interfaces(const ClassFile & cf, ClassLoader *loader);
private:
	InstanceKlass(const InstanceKlass &);
public:
	InstanceKlass(const ClassFile & cf, ClassLoader *loader);
	~InstanceKlass() {};
};

#endif /* INCLUDE_RUNTIME_KLASS_HPP_ */

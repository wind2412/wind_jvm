#ifndef __META_CLASS_H__
#define __META_CLASS_H__

#include <unordered_map>
#include <string>
#include <class_parser.hpp>
#include <memory>

using std::wstring;
using std::unordered_map;
using std::shared_ptr;

class Interfaces {
private:
	unordered_map<wstring, shared_ptr<struct CONSTANT_CS_info>> imap;		// 必须要写一个 struct...... 否则编译报错？
public:
	shared_ptr<CONSTANT_CS_info> getInterface(const wstring & ) const;
};

class Fields {
private:
	
};

class Methods {

};

class Attributes {

};

class MetaClass {

};



#endif
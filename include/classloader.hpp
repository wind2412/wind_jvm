#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include <map>
#include <string>
#include <class_parser.hpp>
#include <jarLister.hpp>
#include <metaclass.hpp>

using std::map;

class ClassFile;

class ClassLoader {};

class MetaClass;

class BootStrapClassLoader : public ClassLoader {
private:
	JarLister jl;
	map<wstring, MetaClass> classmap;
public:
	BootStrapClassLoader() {}
	MetaClass loadClass(const wstring & classname);	// java/util/Map	// java/蛤蛤/ArrayList	// 还是不支持 unicode 了......
};











#endif
#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include <map>
#include <string>
#include <class_parser.hpp>
#include <jarLister.hpp>
#include <metaclass.hpp>
#include <fstream>

using std::map;

class ClassFile;

class ClassLoader {};

class BootStrapClassLoader : public ClassLoader {
private:
	JarLister jl;
	map<wstring, shared_ptr<ClassFile>> classmap;
public:
	BootStrapClassLoader() {}
	shared_ptr<ClassFile> loadClass(const wstring & classname);	// java/util/Map	// java/蛤蛤/ArrayList
};











#endif

#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include <map>
#include <string>
#include <fstream>
#include "class_parser.hpp"
#include "jarLister.hpp"
#include "system_directory.hpp"

using std::map;

class ClassFile;
class InstanceKlass;

class ClassLoader {};

class BootStrapClassLoader : public ClassLoader {
private:
	JarLister jl;
private:
	BootStrapClassLoader() {}
	static BootStrapClassLoader bootstrap;
public:
	static BootStrapClassLoader & get_bootstrap() { return bootstrap; }	// singleton
	shared_ptr<InstanceKlass> loadClass(const wstring & classname);	// load and link class.
};

class MyClassLoader {
private:
	BootStrapClassLoader bs = BootStrapClassLoader::get_bootstrap();
	map<wstring, shared_ptr<InstanceKlass>> classmap;
private:
	MyClassLoader() {};
	static MyClassLoader mloader;
public:
	static MyClassLoader & get_loader() { return mloader; }
	shared_ptr<InstanceKlass> loadClass(const wstring & classname);
};









#endif

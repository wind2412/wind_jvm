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
	BootStrapClassLoader(const BootStrapClassLoader &);
	BootStrapClassLoader& operator= (const BootStrapClassLoader &);
	~BootStrapClassLoader() {}
public:
	static BootStrapClassLoader & get_bootstrap() {
		static BootStrapClassLoader bootstrap;		// 把这句放到 private 中，然后在 classloader.cpp 加上 static 初始化，然后就和 Mayers 条款 4 一样，static 在模块初始化顺序不确定！！会出现相当诡异的结果！！
		return bootstrap;
	}	// singleton
	shared_ptr<InstanceKlass> loadClass(const wstring & classname);	// load and link class.
	void print();
};

class MyClassLoader : public ClassLoader {
private:
	BootStrapClassLoader & bs = BootStrapClassLoader::get_bootstrap();
	map<wstring, shared_ptr<InstanceKlass>> classmap;
private:
	MyClassLoader() {};
	MyClassLoader(const MyClassLoader &);
	MyClassLoader& operator= (const MyClassLoader &);
	~MyClassLoader() {}
public:
	static MyClassLoader & get_loader() {
		static MyClassLoader mloader;
		return mloader;
	}	// singleton
	shared_ptr<InstanceKlass> loadClass(const wstring & classname);
	void print();
};









#endif

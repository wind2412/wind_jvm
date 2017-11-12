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
class Klass;

class ClassLoader {
public:
	virtual shared_ptr<Klass> loadClass(const wstring & classname) = 0;	// load and link class.
	virtual void print() = 0;
	virtual ~ClassLoader() {};	// need to be defined!!
};

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
	shared_ptr<Klass> loadClass(const wstring & classname) override;		// 设计错误。因为还可能 load 数组类以及各种其他，所以必须用 Klass 而不是 InstanceKlass......
	void print() override;
};

class MyClassLoader : public ClassLoader {
private:
	BootStrapClassLoader & bs = BootStrapClassLoader::get_bootstrap();
	map<wstring, shared_ptr<Klass>> classmap;
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
	shared_ptr<Klass> loadClass(const wstring & classname) override;
	void print() override;
};









#endif

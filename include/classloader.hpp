#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include <map>
#include <string>
#include <fstream>
#include <streambuf>
#include "class_parser.hpp"
#include "jarLister.hpp"
#include "system_directory.hpp"
#include "utils/lock.hpp"
#include <iomanip>
#include <list>

using std::map;
using std::list;

class ClassFile;
class Klass;
class ByteStream;
class MirrorOop;
class InstanceKlass;
class ObjArrayOop;

class ClassFile_Pool {
private:
	static Lock & classfile_pool_lock();
private:
	static list<ClassFile *> & classfile_pool();
public:
	static void put(ClassFile *cf);
	static void cleanup();
};

class GC;

class ClassLoader {
	friend GC;
public:
	// the third argument is the Java's ClassLoader, maybe Launcher$AppClassLoader's mirror.
	virtual Klass *loadClass(const wstring & classname, ByteStream * = nullptr, MirrorOop * = nullptr,
										bool = false, InstanceKlass *hostklass = nullptr, ObjArrayOop *cp_patch = nullptr) = 0;	// load and link class.
										// the last three arguments are for: Anonymous klass.
	virtual void print() = 0;
	virtual void cleanup() = 0;
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
		static BootStrapClassLoader bootstrap;
		return bootstrap;
	}	// singleton
	Klass *loadClass(const wstring & classname, ByteStream * = nullptr, MirrorOop * = nullptr,
								bool = false, InstanceKlass * = nullptr, ObjArrayOop * = nullptr) override;
	void print() override;
	void cleanup() override;
};


/**
 * learn from [The C++ Standard Library], Chapter 13.
 */
class ByteStream : public std::streambuf {		// use a derived-streambuf to adapt the istream...
protected:
	char *buf;
	int length;
public:
	ByteStream(char *buf, int length) : buf(buf), length(length) {
		setg(buf, buf, buf + length);
	}
	void print(char splitter = ' ', bool showbase = false) {		// pretty print (useful)
		if (showbase)
			std::wcout << std::showbase;		// print with `0x`.
		for (int i = 0; i < length; i ++) {
			std::wcout << std::hex << +(unsigned char)buf[i] << splitter;
		}
		// set back and print `\n`.
		std::wcout << std::noshowbase << std::dec << std::endl;
	}
};

class MyClassLoader : public ClassLoader {
	friend GC;
private:
	Lock lock;
	BootStrapClassLoader & bs = BootStrapClassLoader::get_bootstrap();
	map<wstring, Klass *> classmap;
	vector<InstanceKlass *> anonymous_klassmap;
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
	Klass *loadClass(const wstring & classname, ByteStream *byte_buf = nullptr, MirrorOop *loader = nullptr,
								bool is_anonymous = false, InstanceKlass *hostklass = nullptr, ObjArrayOop *cp_patch = nullptr) override;
	void print() override;
	void cleanup() override;
	Klass *find_in_classmap(const wstring & classname);
};


#endif

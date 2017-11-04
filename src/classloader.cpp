#include <classloader.hpp>
#include <vm_exceptions.hpp>
#include <memory>

using std::ifstream;
using std::make_shared;

shared_ptr<ClassFile> BootStrapClassLoader::loadClass(const wstring & classname)
{
	wstring target = classname + L".class";
	if (jl.find_file(target)) {	// 在 rt.jar 中，BootStrap 可以 load。
		if (classmap.find(target) != classmap.end()) {	// 已经 load 过
			return classmap[target];
		} else {	// 进行 load
			// parse a ClassFile
			ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong!" << std::endl;
				return nullptr;
			}
			ClassFile cf;
			f >> cf;
			// convert to a MetaClass
			return make_shared<ClassFile>(std::move(cf));
		}
	} else {		// BootStrap 无法加载
		// throw ClassNotFoundExcpetion(ERRMSG, target);	// 不应该抛异常...
		return nullptr;
	}
}

// ClassLoader::
shared_ptr<ClassFile> MyClassLoader::loadClass(const wstring & classname)
{
	shared_ptr<ClassFile> result;
	if((result = bs.loadClass(classname)) != nullptr) {
		return result;
	} else {
		// TODO: 可以加上 classpath。
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {
			return classmap[target];
		} else {
			ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong!" << std::endl;
				return nullptr;
			}
			ClassFile cf;
			f >> cf;
			return make_shared<ClassFile>(std::move(cf));
		}
	}
}

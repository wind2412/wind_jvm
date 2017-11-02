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

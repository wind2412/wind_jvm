#include <memory>
#include "classloader.hpp"
#include "vm_exceptions.hpp"
#include "runtime/klass.hpp"


using std::ifstream;
using std::shared_ptr;
using std::make_shared;

/*===-------------------  BootStrap ClassLoader ----------------------===*/
shared_ptr<InstanceKlass> BootStrapClassLoader::loadClass(const wstring & classname)
{
	wstring target = classname + L".class";
	if (jl.find_file(target)) {	// 在 rt.jar 中，BootStrap 可以 load。
		if (system_classmap.find(target) != system_classmap.end()) {	// 已经 load 过
			return system_classmap[target];
		} else {	// 进行 load
			// parse a ClassFile (load)
			ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong!" << std::endl;
				return nullptr;
			}
			ClassFile cf;
			f >> cf;
			// convert to a MetaClass (link)
			shared_ptr<InstanceKlass> newklass = make_shared<InstanceKlass>(std::move(cf), nullptr);
			system_classmap.insert(make_pair(target, newklass));
			return newklass;
		}
	} else {		// BootStrap 无法加载
		// throw ClassNotFoundExcpetion(ERRMSG, target);	// 不应该抛异常...
		return nullptr;
	}
}

/*===-------------------  My ClassLoader -------------------===*/
shared_ptr<InstanceKlass> MyClassLoader::loadClass(const wstring & classname)
{
	shared_ptr<InstanceKlass> result;
	if((result = bs.loadClass(classname)) != nullptr) {
		return result;
	} else {
		// TODO: 可以加上 classpath。
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {
			return classmap[target];
		} else {
			// bootstrapclassloader load
			shared_ptr<InstanceKlass> get_klass = bs.loadClass(classname);
			if (get_klass != nullptr) return get_klass;
			// else myclassloader load
			ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong!" << std::endl;
				return nullptr;
			}
			ClassFile cf;
			f >> cf;
			// convert to a MetaClass (link)
			shared_ptr<InstanceKlass> newklass = make_shared<InstanceKlass>(std::move(cf), nullptr);
			classmap.insert(make_pair(target, newklass));
			return newklass;
		}
	}
}

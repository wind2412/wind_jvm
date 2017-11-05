#include <memory>
#include "classloader.hpp"
#include "vm_exceptions.hpp"
#include "runtime/klass.hpp"

using std::ifstream;
using std::shared_ptr;
using std::make_shared;

/*===-------------------  BootStrap ClassLoader ----------------------===*/
//BootStrapClassLoader BootStrapClassLoader::bootstrap;	// 见 classloader.hpp::BootStrapClassLoader!! 这里模块之间初始化顺序诡异啊 还是 Mayers 老人家说的对OWO 没想到竟然有一天被我碰上了......QAQ

shared_ptr<InstanceKlass> BootStrapClassLoader::loadClass(const wstring & classname)
{
	assert(jl.find_file(L"java/lang/Object.class")==1);	// 这句是上边 static 模块之间初始化顺序不定实验的残留物。留着吧。
	wstring target = classname + L".class";
	if (jl.find_file(target)) {	// 在 rt.jar 中，BootStrap 可以 load。
		if (system_classmap.find(target) != system_classmap.end()) {	// 已经 load 过
			return system_classmap[target];
		} else {	// 进行 load
			// parse a ClassFile (load)
			ifstream f(wstring_to_utf8(jl.get_sun_dir() + L"/" + target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong! --- at BootStrapClassLoader::loadClass" << std::endl;
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

void BootStrapClassLoader::print()
{
	std::wcout << "===------------ ( BootStrapClassLoader ) Debug TotalClassesPool ---------------===" << std::endl;
	std::cout << "total Classes num: " << system_classmap.size() << std::endl;
	for (auto iter : system_classmap) {
		std::wcout << "  " << iter.first << std::endl;
	}
	std::cout <<  "===----------------------------------------------------------------------------===" << std::endl;
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
				std::cerr << "wrong! --- at MyClassLoader::loadClass" << std::endl;
				return nullptr;
			}
			ClassFile cf;
			f >> cf;
			// convert to a MetaClass (link)
			shared_ptr<InstanceKlass> newklass = make_shared<InstanceKlass>(std::move(cf), this);
			classmap.insert(make_pair(target, newklass));
			return newklass;
		}
	}
}

void MyClassLoader::print()
{
	std::wcout << "===--------------- ( MyClassLoader ) Debug TotalClassesPool ---------------===" << std::endl;
	std::cout << "total Classes num: " << this->classmap.size() << std::endl;
	for (auto iter : this->classmap) {
		std::wcout << "  " << iter.first << std::endl;
	}
	std::cout <<  "===------------------------------------------------------------------------===" << std::endl;
}

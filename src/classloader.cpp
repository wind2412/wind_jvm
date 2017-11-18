#include <memory>
#include <boost/algorithm/string/predicate.hpp>
#include "classloader.hpp"
#include "vm_exceptions.hpp"
#include "runtime/klass.hpp"

using std::ifstream;
using std::shared_ptr;
using std::make_shared;

/*===-------------------  BootStrap ClassLoader ----------------------===*/
//BootStrapClassLoader BootStrapClassLoader::bootstrap;	// 见 classloader.hpp::BootStrapClassLoader!! 这里模块之间初始化顺序诡异啊 还是 Mayers 老人家说的对OWO 没想到竟然有一天被我碰上了......QAQ

shared_ptr<Klass> BootStrapClassLoader::loadClass(const wstring & classname)	// TODO: ... 如果我恶意删掉 java.lang.Object 会怎样......
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
			std::wcout << "===----------------- begin parsing (" << target << ") 's ClassFile in BootstrapClassLoader..." << std::endl;
			shared_ptr<ClassFile> cf(new ClassFile);
			f >> *cf;
			std::wcout << "===----------------- parsing (" << target << ") 's ClassFile end." << std::endl;
			// convert to a MetaClass (link)
			shared_ptr<InstanceKlass> newklass = make_shared<InstanceKlass>(cf, nullptr);
			system_classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
			return newklass;
		}
	} else if (boost::starts_with(classname, L"[")) {		// 一次剥离n层到最里边，加载数组类
		if (system_classmap.find(target) != system_classmap.end()) {	// 已经 load 过
			return system_classmap[target];
		} else {	// 进行 load
			int pos = 0;
			for (; pos < classname.size(); pos ++) {
				if (classname[pos] != L'[') 	break;
			}
			int layer = pos;
			bool is_basic_type = (classname[pos] == L'L') ? false : true;
			if (!is_basic_type) {
				// a. load the spliced class
				wstring temp = classname.substr(layer + 1);		// "Ljava.lang.Object;" --> "java.lang.Object;"
				wstring _true = temp.substr(0, temp.size()-1);	// "java.lang.Object;" --> "java.lang.Object"
				std::wcout << layer << " " << _true << std::endl;
				shared_ptr<InstanceKlass> inner = std::static_pointer_cast<InstanceKlass>(loadClass(_true));		// delete start symbol 'L' like 'Ljava.lang.Object'.
				std::cout << (inner == nullptr) << std::endl;
				if (inner == nullptr)	return nullptr;		// **attention**!! if bootstrap can't load this class, the array must be loaded by myclassloader!!!

				// b. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					shared_ptr<ObjArrayKlass> newklass = make_shared<ObjArrayKlass>(inner, layer, nullptr, nullptr, nullptr);
					system_classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);		// strip one '[' only
					wstring temp_true_inner = temp_inner.substr(0, temp_inner.size()-1);
					shared_ptr<ObjArrayKlass> last_dimension_array = std::static_pointer_cast<ObjArrayKlass>(loadClass(temp_true_inner));		// get last dimension array klass
					shared_ptr<ObjArrayKlass> newklass = make_shared<ObjArrayKlass>(inner, layer, nullptr, last_dimension_array, nullptr);	// use for this dimension array klass
					assert(last_dimension_array->get_higher_dimension() == nullptr);
					last_dimension_array->set_higher_dimension(newklass);
					system_classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
					return newklass;
				}
			} else {
				assert(classname.size() == (layer + 1));	// because it is basic type, so like '[[[C', it must be [layer + 1].
				wstring type_name = classname.substr(pos);
				// b. parse type array
				Type type = get_type(type_name);

				// c. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					shared_ptr<TypeArrayKlass> newklass = make_shared<TypeArrayKlass>(type, layer, nullptr, nullptr, nullptr);
					system_classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);
					shared_ptr<TypeArrayKlass> last_dimension_array = std::static_pointer_cast<TypeArrayKlass>(loadClass(temp_inner));
					shared_ptr<TypeArrayKlass> newklass = make_shared<TypeArrayKlass>(type, layer, nullptr, last_dimension_array, nullptr);
					assert(last_dimension_array->get_higher_dimension() == nullptr);
					last_dimension_array->set_higher_dimension(newklass);
					system_classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
					return newklass;
				}
			}
		}
	} else {		// BootStrap 无法加载: 1. 正在加载一个非 java 类库的类；2. 正在加载一个数组类，而经过层层剥离之后发现它是一个普通类型。
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
shared_ptr<Klass> MyClassLoader::loadClass(const wstring & classname)
{
	shared_ptr<InstanceKlass> result;
	if((result = std::static_pointer_cast<InstanceKlass>(bs.loadClass(classname))) != nullptr) {
		return result;
	} else if (!boost::starts_with(classname, L"[")) {	// not '[[Lcom.zxl.Haha'.
		// TODO: 可以加上 classpath。
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {
			return classmap[target];
		} else {
			ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::cerr << "wrong! --- at MyClassLoader::loadClass" << std::endl;
				return nullptr;
			}
			std::wcout << "===----------------- begin parsing (" << target << ") 's ClassFile in MyClassLoader ..." << std::endl;
			shared_ptr<ClassFile> cf(new ClassFile);
			f >> *cf;
			std::wcout << "===----------------- parsing (" << target << ") 's ClassFile end." << std::endl;
			// convert to a MetaClass (link)
			shared_ptr<InstanceKlass> newklass = make_shared<InstanceKlass>(cf, this);
			classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
			return newklass;
		}
	} else {	// e.g. '[[Lcom.zxl.Haha.   because if it is '[[Ljava.lang.Object', BootStrapClassLoader will load it already at the beginning of this method.
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {	// 已经 load 过
			return classmap[target];
		} else {	// 进行 load
			int pos = 0;
			for (; pos < classname.size(); pos ++) {
				if (classname[pos] != L'[') 	break;
			}
			int layer = pos;
			bool is_basic_type = (classname[pos] == L'L') ? false : true;
			if (!is_basic_type) {
				// a. load the spliced class
				wstring temp = classname.substr(layer+1);		// java.lang.Object;
				wstring _true = temp.substr(0, temp.size()-1);
				std::wcout << layer << " " << _true << std::endl;
				shared_ptr<InstanceKlass> inner = std::static_pointer_cast<InstanceKlass>(loadClass(_true));		// delete start symbol 'L' like 'Ljava.lang.Object'.
				if (inner == nullptr)	return nullptr;		// **attention**!! if bootstrap can't load this class, the array must be loaded by myclassloader!!!

				// b. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					shared_ptr<ObjArrayKlass> newklass = make_shared<ObjArrayKlass>(inner, layer, nullptr, nullptr, nullptr);
					classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);		// strip one '[' only
					wstring temp_true_inner = temp_inner.substr(0, temp_inner.size()-1);
					shared_ptr<ObjArrayKlass> last_dimension_array = std::static_pointer_cast<ObjArrayKlass>(loadClass(temp_true_inner));		// get last dimension array klass
					shared_ptr<ObjArrayKlass> newklass = make_shared<ObjArrayKlass>(inner, layer, nullptr, last_dimension_array, nullptr);	// use for this dimension array klass
					assert(last_dimension_array->get_higher_dimension() == nullptr);
					last_dimension_array->set_higher_dimension(newklass);
					classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
					return newklass;
				}
			} else {
				assert(classname.size() == (layer + 1));	// because it is basic type, so like '[[[C', it must be [layer + 1].
				wstring type_name = classname.substr(pos);
				// b. parse type array
				Type type = get_type(type_name);

				// c. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					shared_ptr<TypeArrayKlass> newklass = make_shared<TypeArrayKlass>(type, layer, nullptr, nullptr, nullptr);
					classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);
					shared_ptr<TypeArrayKlass> last_dimension_array = std::static_pointer_cast<TypeArrayKlass>(loadClass(temp_inner));
					shared_ptr<TypeArrayKlass> newklass = make_shared<TypeArrayKlass>(type, layer, nullptr, last_dimension_array, nullptr);
					assert(last_dimension_array->get_higher_dimension() == nullptr);
					last_dimension_array->set_higher_dimension(newklass);
					classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
					return newklass;
				}
			}
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

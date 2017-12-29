#include <memory>
#include <boost/algorithm/string/predicate.hpp>
#include "classloader.hpp"
#include "runtime/klass.hpp"
#include "utils/utils.hpp"
#include "utils/synchronize_wcout.hpp"
#include "runtime/oop.hpp"
#include "wind_jvm.hpp"

using std::ifstream;
using std::shared_ptr;
using std::make_shared;

Lock & ClassFile_Pool::classfile_pool_lock(){
	static Lock classfile_pool_lock;
	return classfile_pool_lock;
}
list<ClassFile *> & ClassFile_Pool::classfile_pool() {
	static list<ClassFile *> classfile_pool;
	return classfile_pool;
}
void ClassFile_Pool::put(ClassFile *cf) {
	LockGuard lg(classfile_pool_lock());
	classfile_pool().push_back(cf);
}
void ClassFile_Pool::cleanup() {
	LockGuard lg(classfile_pool_lock());
	for (auto iter : classfile_pool()) {
		delete iter;
	}
}

/*===-------------------  BootStrap ClassLoader ----------------------===*/
Klass *BootStrapClassLoader::loadClass(const wstring & classname, ByteStream *, MirrorOop *,
												  bool, InstanceKlass *, ObjArrayOop *)
{
	// add lock simply
	LockGuard lg(system_classmap_lock);
	assert(jl.find_file(L"java/lang/Object.class")==1);
	wstring target = classname + L".class";
	if (jl.find_file(target)) {
		if (system_classmap.find(target) != system_classmap.end()) {	// has been loaded
			return system_classmap[target];
		} else {	// load
			// parse a ClassFile (load)
			ifstream f(wstring_to_utf8(jl.get_sun_dir() + L"/" + target).c_str(), std::ios::binary);
			if(!f.is_open()) {
				std::wcerr << "wrong! --- at BootStrapClassLoader::loadClass" << std::endl;
				exit(-1);
			}
#ifdef DEBUG
			sync_wcout{} << "===----------------- begin parsing (" << target << ") 's ClassFile in BootstrapClassLoader..." << std::endl;
#endif
			ClassFile *cf = new ClassFile;
			ClassFile_Pool::put(cf);
			f >> *cf;
#ifdef DEBUG
			sync_wcout{} << "===----------------- parsing (" << target << ") 's ClassFile end." << std::endl;
#endif
			// convert to a MetaClass (link)
			InstanceKlass *newklass = new InstanceKlass(cf, nullptr);
			system_classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
			return newklass;
		}
	} else if (boost::starts_with(classname, L"[")) {
		if (system_classmap.find(target) != system_classmap.end()) {	// has been loaded
			return system_classmap[target];
		} else {	// load
			int pos = 0;
			for (; pos < classname.size(); pos ++) {
				if (classname[pos] != L'[') 	break;
			}
			int layer = pos;
			bool is_basic_type = (classname[pos] == L'L') ? false : true;
			if (!is_basic_type) {
				// a. load the spliced class
				wstring temp = classname.substr(layer + 1);		// "Ljava/lang/Object;" --> "java/lang/Object;"
				wstring _true = temp.substr(0, temp.size()-1);	// "java/lang/Object;" --> "java/lang/Object"
				InstanceKlass *inner = ((InstanceKlass *)loadClass(_true));		// delete start symbol 'L' like 'Ljava/lang/Object'.
				if (inner == nullptr)	return nullptr;		// **attention**!! if bootstrap can't load this class, the array must be loaded by myclassloader!!!

				// b. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					ObjArrayKlass * newklass = new ObjArrayKlass(inner, layer, nullptr, nullptr, nullptr);
					system_classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_true_inner = classname.substr(1);		// strip one '[' only
					ObjArrayKlass * last_dimension_array = ((ObjArrayKlass *)loadClass(temp_true_inner));		// get last dimension array klass
					ObjArrayKlass * newklass = new ObjArrayKlass(inner, layer, nullptr, last_dimension_array, nullptr);	// use for this dimension array klass
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
					TypeArrayKlass *newklass = new TypeArrayKlass(type, layer, nullptr, nullptr, nullptr);
					system_classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);
					TypeArrayKlass *last_dimension_array = ((TypeArrayKlass *)loadClass(temp_inner));
					TypeArrayKlass *newklass = new TypeArrayKlass(type, layer, nullptr, last_dimension_array, nullptr);
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
	} else {
		// throw ClassNotFoundExcpetion(ERRMSG, target);
		return nullptr;
	}
}

void BootStrapClassLoader::print()
{
	sync_wcout{} << "===------------ ( BootStrapClassLoader ) Debug TotalClassesPool ---------------===" << std::endl;
	LockGuard lg(system_classmap_lock);
	sync_wcout{} << "total Classes num: " << system_classmap.size() << std::endl;
	for (auto iter : system_classmap) {
		sync_wcout{} << "  " << iter.first << std::endl;
	}
	sync_wcout{} << "===----------------------------------------------------------------------------===" << std::endl;
}

void BootStrapClassLoader::cleanup()
{
	LockGuard lg(system_classmap_lock);
	for (auto iter : system_classmap) {
		delete iter.second;
	}
}

/*===-------------------  My ClassLoader -------------------===*/
Klass *MyClassLoader::loadClass(const wstring & classname, ByteStream *byte_buf, MirrorOop *loader_mirror,
										   bool is_anonymous, InstanceKlass *hostklass, ObjArrayOop *cp_patch)
{
	LockGuard lg(this->lock);
	InstanceKlass *result;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) loading ... [" << classname << "]" << std::endl;		// delete
#endif
	if (is_anonymous) {
		assert(byte_buf != nullptr);
		std::istream *stream;
		ClassFile *cf(new ClassFile);
		std::istream s(byte_buf);											// use `istream`, the parent of `ifstream`.
#ifdef DEBUG
		sync_wcout{} << "===----------------- begin parsing (" << classname << ") 's ClassFile in MyClassLoader ..." << std::endl;
#endif

		s >> *cf;

#ifdef DEBUG
		sync_wcout{} << "===----------------- parsing (" << classname << ") 's ClassFile end." << std::endl;
#endif
		// make patches first!
		if (cp_patch != nullptr) {
			auto pool = cf->constant_pool;
			for (int i = 1; i < cp_patch->get_length(); i ++) {	// for cp_patch: 0 is not used.
				if ((*cp_patch)[i] == nullptr) {
					continue;		// this rt_pool[i] do not need patch
				} else {		// patch.
					auto patch_type = (*cp_patch)[i]->get_klass();
//					std::wcout << pool[i-1]->tag << std::endl;		// delete
					switch (pool[i-1]->tag) {
						case CONSTANT_Class:{
							if (patch_type->get_name() == L"java/lang/Class") {		// Class patch.
								assert(false);
							} else {
								assert(false);	// didn't support yet.
							}
							break;
						}
						case CONSTANT_String:{	// return is okay OWO.
							break;
						}
						default:{
							assert(false);
						}
					}

				}
			}
		}

		// convert to a MetaClass (link)
		InstanceKlass *newklass = new InstanceKlass(cf, this /*nullptr*/, loader_mirror);

		// intercept：VM Anonymous Klass's bytecode。
//		if (newklass->get_name() == L"Test13$$Lambda$1") {
//			byte_buf->print(',', true);
//		}

		// set the hostklass.
		newklass->set_hostklass(hostklass);		// set hostklass...
#ifdef KLASS_DEBUG
BootStrapClassLoader::get_bootstrap().print();
MyClassLoader::get_loader().print();
#endif
		anonymous_klassmap.push_back(newklass);
		return newklass;
	} else if((result = ((InstanceKlass *)bs.loadClass(classname))) != nullptr) {		// use BootStrap to load first.
		return result;
	} else if (!boost::starts_with(classname, L"[")) {	// not '[[Lcom/zxl/Haha'.
		// TODO: 可以加上 classpath。
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {
			return classmap[target];
		} else {
			std::istream *stream;
			ClassFile *cf = new ClassFile;
			ClassFile_Pool::put(cf);
			if (byte_buf == nullptr) {
				ifstream f(wstring_to_utf8(target).c_str(), std::ios::binary);		// use `ifstream`
				if(!f.is_open()) {
//					for (auto iter : wind_jvm::threads()) {		// get now pthread_t's vm_thread.
//						if (iter.get_tid() == pthread_self()) {
//							iter.get_stack_trace();
//						}
//					}

					// VM Anonymous Klass will go here.
					auto iter = std::find_if(anonymous_klassmap.begin(), anonymous_klassmap.end(), [&classname](InstanceKlass *anonymous_klass) {
						if (anonymous_klass->get_name() == classname) {
							return true;
						} else {
							return false;
						}
					});

					if (iter == anonymous_klassmap.end()) {
						std::wcerr << "wrong! --- at MyClassLoader::loadClass" << std::endl;
						exit(-1);
					} else {
						return *iter;
					}

				}
#ifdef DEBUG
				sync_wcout{} << "===----------------- begin parsing (" << target << ") 's ClassFile in MyClassLoader ..." << std::endl;
#endif
				f >> *cf;
			} else {		// use ByteBuffer:
				std::istream s(byte_buf);											// use `istream`, the parent of `ifstream`.

				// intercept
//				if (classname == L"java/lang/invoke/BoundMethodHandle$Species_LL") {
//					byte_buf->print(',', true);
//				}

#ifdef DEBUG
				sync_wcout{} << "===----------------- begin parsing (" << target << ") 's ClassFile in MyClassLoader ..." << std::endl;
#endif
				s >> *cf;
			}
//			std::wcout << "===----------------- begin parsing (" << target << ") 's ClassFile in MyClassLoader ..." << std::endl;
//			(*stream) >> *cf;
#ifdef DEBUG
			sync_wcout{} << "===----------------- parsing (" << target << ") 's ClassFile end." << std::endl;
#endif
			// convert to a MetaClass (link)
			InstanceKlass *newklass = new InstanceKlass(cf, this, loader_mirror);	// set the Java ClassLoader's mirror!!!
			classmap.insert(make_pair(target, newklass));
#ifdef KLASS_DEBUG
BootStrapClassLoader::get_bootstrap().print();
MyClassLoader::get_loader().print();
#endif
			return newklass;
		}
	} else {	// e.g. '[[Lcom/zxl/Haha.   because if it is '[[Ljava/lang/Object', BootStrapClassLoader will load it already at the beginning of this method.
		wstring target = classname + L".class";
		if (classmap.find(target) != classmap.end()) {	// has been loaded
			return classmap[target];
		} else {	// load
			int pos = 0;
			for (; pos < classname.size(); pos ++) {
				if (classname[pos] != L'[') 	break;
			}
			int layer = pos;
			bool is_basic_type = (classname[pos] == L'L') ? false : true;
			if (!is_basic_type) {
				// a. load the spliced class
				wstring temp = classname.substr(layer+1);		// java/lang/Object;
				wstring _true = temp.substr(0, temp.size()-1);
				InstanceKlass *inner = ((InstanceKlass *)loadClass(_true));		// delete start symbol 'L' like 'Ljava/lang/Object'.
				if (inner == nullptr)	return nullptr;		// **attention**!! if bootstrap can't load this class, the array must be loaded by myclassloader!!!

				// b. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					ObjArrayKlass * newklass = new ObjArrayKlass(inner, layer, nullptr, nullptr, nullptr, loader_mirror);	// set the Java ClassLoader's mirror!!!
					classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);		// strip one '[' only
					wstring temp_true_inner = temp_inner.substr(0, temp_inner.size()-1);
					ObjArrayKlass * last_dimension_array = ((ObjArrayKlass *)loadClass(temp_true_inner));		// get last dimension array klass
					ObjArrayKlass * newklass = new ObjArrayKlass(inner, layer, nullptr, last_dimension_array, nullptr, loader_mirror);	// use for this dimension array klass
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
				assert(false);
				assert(classname.size() == (layer + 1));	// because it is basic type, so like '[[[C', it must be [layer + 1].
				wstring type_name = classname.substr(pos);
				// b. parse type array
				Type type = get_type(type_name);

				// c. recursively load the [, [[, [[[ ... until this, maybe [[[[[.
				if (layer == 1) {
					TypeArrayKlass *newklass = new TypeArrayKlass(type, layer, nullptr, nullptr, nullptr);
					classmap.insert(make_pair(target, newklass));
					return newklass;
#ifdef KLASS_DEBUG
	BootStrapClassLoader::get_bootstrap().print();
	MyClassLoader::get_loader().print();
#endif
				} else {
					wstring temp_inner = classname.substr(1);
					TypeArrayKlass *last_dimension_array = ((TypeArrayKlass *)loadClass(temp_inner));
					TypeArrayKlass *newklass = new TypeArrayKlass(type, layer, nullptr, last_dimension_array, nullptr);
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
	sync_wcout{} << "===--------------- ( MyClassLoader ) Debug TotalClassesPool ---------------===" << std::endl;
	LockGuard lg(this->lock);
	sync_wcout{} << "total Classes num: " << this->classmap.size() << std::endl;
	for (auto iter : this->classmap) {
		sync_wcout{} << "  " << iter.first << std::endl;
	}
	sync_wcout{} << "===------------------------------------------------------------------------===" << std::endl;
}

Klass *MyClassLoader::find_in_classmap(const wstring & classname)
{
	LockGuard lg(this->lock);
	auto iter = this->classmap.find(classname);
	if (iter == this->classmap.end()) {
		return nullptr;
	} else {
		return iter->second;
	}
}

void MyClassLoader::cleanup()
{
	LockGuard lg(this->lock);
	for (auto iter : anonymous_klassmap) {
		delete iter;
	}
	for (auto iter : classmap) {
		delete iter.second;
	}
}

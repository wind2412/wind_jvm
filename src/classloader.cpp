#include <classloader.hpp>

MetaClass BootStrapClassLoader::loadClass(const wstring & classname) 
{
	if (jl.find_file(classname + L".class")) {

	}
}

// ClassLoader::
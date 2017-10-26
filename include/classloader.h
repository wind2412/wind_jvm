#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include <map>
#include <string>
#include <class_parser.h>

using std::string;
using std::map;

class ClassLoader {
private:
	map<string, ClassFile> classmap;
public:
	ClassLoader();
	void loadClass();
};











#endif
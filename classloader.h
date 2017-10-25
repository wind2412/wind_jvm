#ifndef __CLASS_LOADER_H__
#define __CLASS_LOADER_H__

#include "class_parser.h"
#include <map>
#include <string>

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
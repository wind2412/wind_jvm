#ifndef __VM_EXCEPTIONS_H__
#define __VM_EXCEPTIONS_H__

#include <string>
#include <sstream>
#include "utils/utils.hpp"

#define ERRMSG __FILE__, __LINE__

using std::string;
using std::wstring;
using std::wstringstream;

class exception {
protected:
	wstringstream ss;
public:
	explicit exception(const char *filename, int line) {
		ss << "at " << utf8_to_wstring(string(filename)) << ", line: " << line;
	}
	virtual wstring what() {
		return ss.str();
	}
};

class ClassNotFoundExcpetion : public exception {
private:
	const wstring clazz;
public:
	ClassNotFoundExcpetion(const char *filename, int line, const wstring & clazz) : exception(filename, line), clazz(clazz) {}
	virtual wstring what() override {
		return dynamic_cast<wstringstream &>(ss << clazz).str();
	}
};


#endif

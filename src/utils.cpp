#include "utils.hpp"
#include <cassert>
#include <iostream>

std::string wstring_to_utf8 (const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

std::wstring utf8_to_wstring (const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

int parse_field_descriptor(const std::wstring & descriptor)
{
	if(descriptor.size() == 1) {
		switch(descriptor[0]) {
			case L'B':	// byte
				return 1;
			case L'C':	// char
				return 2;
			case L'D':	// double
				return 8;
			case L'F':	// float
				return 4;
			case L'I':	// int
				return 4;
			case L'J':	// long
				return 8;
			case L'S':	// short
				return 2;
			case L'Z':	// boolean
				return 1;
			default:
				std::cerr << "can't get here in <utils.cpp>::parse_field_descriptor!" << std::endl;
				assert(false);
				return -1;
		}
	} else {
		return 8;	// [I or Ljava.lang.String.  Array && Reference is pointer in x64.
	}
}

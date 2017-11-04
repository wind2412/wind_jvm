#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <locale>
#include <codecvt>

// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str);

// convert UTF-8 to wstring
std::wstring utf8_to_wstring (const std::string& str);

// parse field descriptor to get length (or type), like I, B, D, [I, [[I, [[Ljava.lang.String; .etc
int parse_field_descriptor(const std::wstring & descriptor);

#endif

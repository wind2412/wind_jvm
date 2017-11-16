/*
 * system_directory.hpp
 *
 *  Created on: 2017年11月4日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_SYSTEM_DIRECTORY_HPP_
#define INCLUDE_SYSTEM_DIRECTORY_HPP_

#include <unordered_map>
#include <string>
#include <memory>

using std::wstring;
using std::unordered_map;
using std::shared_ptr;

class Klass;

extern unordered_map<wstring, shared_ptr<Klass>> system_classmap;		// java/lang/Object.class

#endif /* INCLUDE_SYSTEM_DIRECTORY_HPP_ */

/*
 * java_security_AccessController.hpp
 *
 *  Created on: 2017年11月25日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JAVA_SECURITY_ACCESSCONTROLLER_HPP_
#define INCLUDE_NATIVE_JAVA_SECURITY_ACCESSCONTROLLER_HPP_

#include "runtime/oop.hpp"
#include <list>

using std::list;
using std::string;

void JVM_DoPrivileged (list<Oop*>& _stack);
void JVM_GetStackAccessControlContext(list<Oop *> & _stack);



void *java_security_accesscontroller_search_method(const wstring & signature);

#endif /* INCLUDE_NATIVE_JAVA_SECURITY_ACCESSCONTROLLER_HPP_ */

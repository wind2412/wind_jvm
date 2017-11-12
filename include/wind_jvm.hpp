/*
 * wind_jvm.hpp
 *
 *  Created on: 2017年11月11日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_WIND_JVM_HPP_
#define INCLUDE_WIND_JVM_HPP_

#include "runtime/bytecodeEngine.hpp"
#include "classloader.hpp"
#include <string>
#include <vector>

using std::wstring;
using std::vector;

class wind_jvm {
private:
	// TODO: pthread
	wstring main_class_name;
	vector<StackFrame> vm_stack;
	int rsp;			// offset in [current, valid] vm_stack
	uint8_t *pc;		// pc, pointing to the code segment: inside the Method->code.
public:
	wind_jvm(const wstring & main_class_name, const vector<wstring> & argv);	// TODO: arguments! String[] !!!
};





#endif /* INCLUDE_WIND_JVM_HPP_ */

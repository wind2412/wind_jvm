/*
 * os.cpp
 *
 *  Created on: 2017年11月30日
 *      Author: zhengxiaolin
 */

#include <cassert>
#include <unistd.h>
#include "utils/os.hpp"

// for support of CAS (Unsafe)

//#if (defined (__APPLE__))
//#include <sys/types.h>
//#include <sys/sysctl.h>
//#elif (defined (__linux__))
//#include <linux/sysctl.h>
//#endif

int get_cpu_nums() {
	static int cpu_nums = sysconf(_SC_NPROCESSORS_CONF);		// global cache
	assert(cpu_nums != 0);
	return cpu_nums;
}

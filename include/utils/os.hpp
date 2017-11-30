/*
 * os.hpp
 *
 *  Created on: 2017年11月30日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_UTILS_OS_HPP_
#define INCLUDE_UTILS_OS_HPP_

int get_cpu_nums();

// CAS, from x86 assembly (e.g. linux kernel), and this code is from openjdk.
inline int cmpxchg(int exchange_value, volatile int *dest, int compare_value)
{
	int mp = get_cpu_nums() > 1 ? get_cpu_nums() : 0;
	__asm__ volatile("cmp $0, %4; je 1f; lock; 1: cmpxchgl %1,(%3)"
	                   : "=a"(exchange_value)
	                   : "r"(exchange_value), "a"(compare_value), "r"(dest), "r"(mp)
	                   : "cc", "memory");
	return exchange_value;		// 把原先 dest 里边的 (和 compare_value 相等，目前已经被换到 exchange_value 中去) 值 return。
}


#endif /* INCLUDE_UTILS_OS_HPP_ */

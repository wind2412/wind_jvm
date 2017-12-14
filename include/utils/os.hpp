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

inline long cmpxchg(long exchange_value, volatile long *dest, long compare_value)
{
	int mp = get_cpu_nums() > 1 ? get_cpu_nums() : 0;
	__asm__ volatile("cmp $0, %4; je 1f; lock; 1: cmpxchgq %1,(%3)"
	                   : "=a"(exchange_value)
	                   : "r"(exchange_value), "a"(compare_value), "r"(dest), "r"(mp)
	                   : "cc", "memory");
	return exchange_value;		// 把原先 dest 里边的 (和 compare_value 相等，目前已经被换到 exchange_value 中去) 值 return。
}

inline void fence()
{
	// barrier. I use boost::atomic//ops_gcc_x86.hpp's method.	// TODO: 也有理解不上的地方。acquire 语义究竟是被用于什么地方？为什么用于 LoadLoad 和 LoadStore (x86下)？ 还有待学习啊......
	// openjdk says `mfence` is always expensive...
	__asm__ volatile (			// TODO: 这里同样。mfence 是屏障指令；lock;nop; 也是(虽然手册规范不让这么写，不过后边的语义就是nop)。不过为什么 openjdk AccessOrder 实现中 AMD64 要 addq？
#ifdef __x86_64__
			"mfence;"
#else
			"lock; addl $0, (%%esp);"
#endif
			:::"memory"
	);
}

// from openjdk: OrderAccess
inline void release()
{
	// Avoid hitting the same cache-line from different threads.
	// TODO: 有待学习......
	volatile int local_dummy = 0;
}

// from openjdk: OrderAccess
inline void acquire()
{
	volatile intptr_t local_dummy;
	__asm__ volatile ("movq 0(%%rsp), %0" : "=r" (local_dummy) : : "memory");		// 仅仅是把一个随便的值传了出来...... 稍稍有些改动......因为 movl  %esp. intptr_t 竟然报错...
}


#endif /* INCLUDE_UTILS_OS_HPP_ */

/*
 * thread.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "runtime/thread.hpp"
#include "utils/synchronize_wcout.hpp"
#include "wind_jvm.hpp"

Lock & ThreadTable::get_lock()
{
	static Lock lock;
	return lock;
}

unordered_map<pthread_t, pair<int, InstanceOop *>> & ThreadTable::get_thread_table()
{
	static unordered_map<pthread_t, pair<int, InstanceOop *>> thread_table;
	return thread_table;
}

void ThreadTable::add_a_thread(pthread_t tid, InstanceOop *_thread)
{
	LockGuard lg(get_lock());

	// bug report: unordered_map 中如果有相同元素的话...... insert 会失败，返回 false!!...... 唉...... 而用下标访问就没问题。
	// C++ 17 新增了一个 insert_or_assign() 比较好...QAQ
//	get_thread_table().insert(make_pair(tid, make_pair(get_thread_table().size(), _thread)));		// Override!!!! because tid maybe the same...?
	int number = get_thread_table().size();
	if (get_thread_table().insert(make_pair(tid, make_pair(get_thread_table().size(), _thread))).second == false) {	// 如果原先已经插入了的话，那么就复用原先的 thread_no.
		number = get_thread_table()[tid].first;
	}
	get_thread_table()[tid] = make_pair(get_thread_table().size(), _thread);		// Override!!!! because tid maybe the same...?
					// 已解决： 这里有可能会出故障的。因为设置的时候是设置 size，所以如果要是重复插入了两次，就不好弄了(因为 pthread 会复用原来的线程 pthread_t)。size 就会不准确。不过 size 仅仅用于输出，所以没有别的用处。
					// 不过 fix 了这个问题带来的小代价就是，线程的 thread_no 可能不按照顺序了。没准最新插入的线程因为 pthread_t 复用，而复用了原来的 thread_no。所以这样的话，
					// 可能原先那个 thread_no 是 0，而这个本来应该是 3，但是复用之后还是 0。因此，输出的颜色只能作为分辨 “不同线程” 来用，
					// 但是绝对不能当做 “分辨那个线程是第一个产生的线程，哪个线程是后产生的线程” 来用！！
}

void ThreadTable::remove_a_thread(pthread_t tid)
{
	LockGuard lg(get_lock());
	get_thread_table().erase(tid);
}

int ThreadTable::get_threadno(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return (*iter).second.first;
	}
	return -1;
}

bool ThreadTable::is_in(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return true;
	}
	return false;
}

InstanceOop * ThreadTable::get_a_thread(pthread_t tid)
{
	LockGuard lg(get_lock());
	auto iter = get_thread_table().find(tid);
	if (iter != get_thread_table().end()) {
		return (*iter).second.second;
	}
	return nullptr;
}

bool ThreadTable::detect_thread_death(pthread_t tid)
{
	LockGuard lg(get_lock());
	int ret = pthread_kill(tid, 0);
	if (ret == 0) {
		return false;
	} else if (ret == ESRCH) {
		return true;
	} else {
		std::wcerr << "wrong!" << std::endl;		// maybe the EINVAL...
		assert(false);
	}
}

void ThreadTable::print_table()
{
//#ifdef DEBUG
	sync_wcout::set_switch(true);
	sync_wcout{} << "===------------- ThreadTable ----------------===" << std::endl;		// TODO: 这里，sync_wcout 会 dead lock ??????
	for (auto iter : get_thread_table()) {
		sync_wcout{} << "pthread_t :[" << iter.first << "], is the [" << iter.second.first << "] thread, Thread Oop address: [" << iter.second.second << "]" << std::endl;
	}
	sync_wcout{} << "===------------------------------------------===" << std::endl;
//#endif
}

void ThreadTable::kill_all_except_main_thread(pthread_t main_tid)
{
	for (auto iter : get_thread_table()) {
		if (iter.first == main_tid)	continue;
		else {
//				pthread_cancel(iter.first);
			if (pthread_kill(iter.first, SIGINT) == -1) {
				assert(false);
			}
		}
	}
}

void ThreadTable::wake_up_all_threads_force()
{
	signal_all_thread();
}

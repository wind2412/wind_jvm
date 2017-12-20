/*
 * thread.cpp
 *
 *  Created on: 2017年12月20日
 *      Author: zhengxiaolin
 */

#include "runtime/thread.hpp"
#include "utils/synchronize_wcout.hpp"

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

	print_table();

	// bug report: unordered_map 中如果有相同元素的话...... insert 会失败，返回 false!!...... 唉...... 而用下标访问就没问题。
	// C++ 17 新增了一个 insert_or_assign() 比较好...QAQ
//	get_thread_table().insert(make_pair(tid, make_pair(get_thread_table().size(), _thread)));		// Override!!!! because tid maybe the same...?
	get_thread_table()[tid] = make_pair(get_thread_table().size(), _thread);		// Override!!!! because tid maybe the same...?
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

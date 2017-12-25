/*
 * sun_misc_signal.cpp
 *
 *  Created on: 2017年12月1日
 *      Author: zhengxiaolin
 */

#include "native/sun_misc_signal.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include "wind_jvm.hpp"
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include <signal.h>
#include <unordered_map>

static unordered_map<wstring, void*> methods = {
    {L"findSignal:(" STR ")I",				(void *)&JVM_FindSignal},
    {L"handle0:(IJ)J",						(void *)&JVM_Handle0},
};

#ifdef __APPLE__
static std::unordered_map<wstring, int> siglabels {				// from jvm_bsd.cpp
  /* derived from /usr/include/bits/signum.h on RH7.2 */
  {L"HUP",        SIGHUP},         /* Hangup (POSIX).  */
  {L"INT",        SIGINT},         /* Interrupt (ANSI).  */
  {L"QUIT",       SIGQUIT},        /* Quit (POSIX).  */
  {L"ILL",        SIGILL},         /* Illegal instruction (ANSI).  */
  {L"TRAP",       SIGTRAP},        /* Trace trap (POSIX).  */
  {L"ABRT",       SIGABRT},        /* Abort (ANSI).  */
  {L"EMT",        SIGEMT},         /* EMT trap  */
  {L"FPE",        SIGFPE},         /* Floating-point exception (ANSI).  */
  {L"KILL",       SIGKILL},        /* Kill, unblockable (POSIX).  */
  {L"BUS",        SIGBUS},         /* BUS error (4.2 BSD).  */
  {L"SEGV",       SIGSEGV},        /* Segmentation violation (ANSI).  */
  {L"SYS",        SIGSYS},         /* Bad system call. Only on some Bsden! */
  {L"PIPE",       SIGPIPE},        /* Broken pipe (POSIX).  */
  {L"ALRM",       SIGALRM},        /* Alarm clock (POSIX).  */
  {L"TERM",       SIGTERM},        /* Termination (ANSI).  */
  {L"URG",        SIGURG},         /* Urgent condition on socket (4.2 BSD).  */
  {L"STOP",       SIGSTOP},        /* Stop, unblockable (POSIX).  */
  {L"TSTP",       SIGTSTP},        /* Keyboard stop (POSIX).  */
  {L"CONT",       SIGCONT},        /* Continue (POSIX).  */
  {L"CHLD",       SIGCHLD},        /* Child status has changed (POSIX).  */
  {L"TTIN",       SIGTTIN},        /* Background read from tty (POSIX).  */
  {L"TTOU",       SIGTTOU},        /* Background write to tty (POSIX).  */
  {L"IO",         SIGIO},          /* I/O now possible (4.2 BSD).  */
  {L"XCPU",       SIGXCPU},        /* CPU limit exceeded (4.2 BSD).  */
  {L"XFSZ",       SIGXFSZ},        /* File size limit exceeded (4.2 BSD).  */
  {L"VTALRM",     SIGVTALRM},      /* Virtual alarm clock (4.2 BSD).  */
  {L"PROF",       SIGPROF},        /* Profiling alarm clock (4.2 BSD).  */
  {L"WINCH",      SIGWINCH},       /* Window size change (4.3 BSD, Sun).  */
  {L"INFO",       SIGINFO},        /* Information request.  */
  {L"USR1",       SIGUSR1},        /* User-defined signal 1 (POSIX).  */
  {L"USR2",       SIGUSR2},        /* User-defined signal 2 (POSIX).  */
};
#elif defined __linux__
static std::unordered_map<wstring, int> siglabels {
  /* derived from /usr/include/bits/signum.h on RH7.2 */
  {L"HUP",        SIGHUP},         /* Hangup (POSIX).  */
  {L"INT",        SIGINT},         /* Interrupt (ANSI).  */
  {L"QUIT",       SIGQUIT},        /* Quit (POSIX).  */
  {L"ILL",        SIGILL},         /* Illegal instruction (ANSI).  */
  {L"TRAP",       SIGTRAP},        /* Trace trap (POSIX).  */
  {L"ABRT",       SIGABRT},        /* Abort (ANSI).  */
  {L"IOT",        SIGIOT},         /* IOT trap (4.2 BSD).  */
  {L"BUS",        SIGBUS},         /* BUS error (4.2 BSD).  */
  {L"FPE",        SIGFPE},         /* Floating-point exception (ANSI).  */
  {L"KILL",       SIGKILL},        /* Kill, unblockable (POSIX).  */
  {L"USR1",       SIGUSR1},        /* User-defined signal 1 (POSIX).  */
  {L"SEGV",       SIGSEGV},        /* Segmentation violation (ANSI).  */
  {L"USR2",       SIGUSR2},        /* User-defined signal 2 (POSIX).  */
  {L"PIPE",       SIGPIPE},        /* Broken pipe (POSIX).  */
  {L"ALRM",       SIGALRM},        /* Alarm clock (POSIX).  */
  {L"TERM",       SIGTERM},        /* Termination (ANSI).  */
#ifdef SIGSTKFLT
  {L"STKFLT",     SIGSTKFLT},      /* Stack fault.  */
#endif
  {L"CLD",        SIGCLD},         /* Same as SIGCHLD (System V).  */
  {L"CHLD",       SIGCHLD},        /* Child status has changed (POSIX).  */
  {L"CONT",       SIGCONT},        /* Continue (POSIX).  */
  {L"STOP",       SIGSTOP},        /* Stop, unblockable (POSIX).  */
  {L"TSTP",       SIGTSTP},        /* Keyboard stop (POSIX).  */
  {L"TTIN",       SIGTTIN},        /* Background read from tty (POSIX).  */
  {L"TTOU",       SIGTTOU},        /* Background write to tty (POSIX).  */
  {L"URG",        SIGURG},         /* Urgent condition on socket (4.2 BSD).  */
  {L"XCPU",       SIGXCPU},        /* CPU limit exceeded (4.2 BSD).  */
  {L"XFSZ",       SIGXFSZ},        /* File size limit exceeded (4.2 BSD).  */
  {L"VTALRM",     SIGVTALRM},      /* Virtual alarm clock (4.2 BSD).  */
  {L"PROF",       SIGPROF},        /* Profiling alarm clock (4.2 BSD).  */
  {L"WINCH",      SIGWINCH},       /* Window size change (4.3 BSD, Sun).  */
  {L"POLL",       SIGPOLL},        /* Pollable event occurred (System V).  */
  {L"IO",         SIGIO},          /* I/O now possible (4.2 BSD).  */
  {L"PWR",        SIGPWR},         /* Power failure restart (System V).  */
#ifdef SIGSYS
  {L"SYS",        SIGSYS},         /* Bad system call. Only on some Linuxen! */
#endif
};
#endif


void default_user_handler_for_all_sigs(int signo)
{
	if (signo == SIGINT) {
		exit(0);
	}
}

void JVM_FindSignal(list<Oop *> & _stack){		// static
	InstanceOop *str = (InstanceOop *)_stack.front();	_stack.pop_front();
	auto iter = siglabels.find(java_lang_string::stringOop_to_wstring(str));
	assert(iter != siglabels.end());
	_stack.push_back(new IntOop(iter->second));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) find [" << java_lang_string::stringOop_to_wstring(str) << "] and get sig number: [" << iter->second << "]." << std::endl;
#endif
}

void JVM_Handle0(list<Oop *> & _stack){		// static
	int signo = ((IntOop *)_stack.front())->value;	_stack.pop_front();
	long fake_handler_no = ((LongOop *)_stack.front())->value;	_stack.pop_front();	// must be 0, 1, 2 because of Signal.handle() 's filter.

	void *fake_new_handler = (fake_handler_no == 2) ? (void *)default_user_handler_for_all_sigs : (void *)fake_handler_no;
	switch(signo) {	// from jvm_bsd.cpp
		case SIGUSR1:
		case SIGFPE:
		case SIGKILL:
		case SIGSEGV:
		case SIGQUIT:{
			_stack.push_back(new LongOop(-1));		// wrong. can't modify these signals!
			return;
		}

		case SIGINT:{
			_stack.push_back(new LongOop(2));		// 魔改：用我自己的 handler 了......
			return;
		}

		case SIGHUP:
		case SIGTERM:{
			// detect whether the sig is `ignored`. if not, set the handler. if yes, return 1.
			struct sigaction oact;
			int ret = sigaction(signo, nullptr, &oact);
			assert(ret != -1);		// if we only care about the `current handler` and don't want to set a new handler, we can pass a `nullptr` because we don't want to set a new handler, only want it to return the current handler..
#ifdef __APPLE__
			void *old_handler = (void *)oact.__sigaction_u.__sa_handler;		// this is the `current handler`.
#elif defined __linux__
			void *old_handler = (void *)oact.sa_handler;
#endif
			if (old_handler == (void *)SIG_IGN) {
				_stack.push_back(new LongOop(1));
			} else {
				// go next.
			}
		}
	}

	// register the handler now.
	struct sigaction act, oact;
	sigfillset(&act.sa_mask);
	act.sa_flags = SA_RESTART;		// 如果信号被中断给中断了，那么中断结束之后会自动继续执行 handler。
#ifdef __APPLE__
	act.__sigaction_u.__sa_handler = (void (*)(int))fake_new_handler;		// 由于 fake_new_handler 只能是 0，1，2。0 和 1 分别是 SIG_DFL 和 SIG_IGN，分别是 (void (*)(int))0 和 (void (*)(int))1，不用变。
#elif defined __linux__
	act.sa_handler = (void (*)(int))fake_new_handler;
#endif

#ifdef __APPLE__
	void *old_handler = (void *)oact.__sigaction_u.__sa_handler;
#elif defined __linux__
	void *old_handler = (void *)oact.sa_handler;
#endif
	if (sigaction(signo, &act, &oact) != -1) {
		if (old_handler == (void *)default_user_handler_for_all_sigs) {
			_stack.push_back(new LongOop(2));
		} else {
			_stack.push_back(new LongOop((long)old_handler));
		}
	} else {
		_stack.push_back(new LongOop(-1));	// error.
	}

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) set signo: [" << signo << "]'s handler to a new handler: [(void (*)(int)" << std::hex << ((LongOop *)_stack.back())->value << "]." << std::endl;
#endif

}



// 返回 fnPtr.
void *sun_misc_signal_search_method(const wstring & signature)
{
	auto iter = methods.find(signature);
	if (iter != methods.end()) {
		return (*iter).second;
	}
	return nullptr;
}




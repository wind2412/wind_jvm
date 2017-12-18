#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <locale>
#include <codecvt>

/*===-----------------  Wstring ---------------------===*/
// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str);

// convert UTF-8 to wstring
std::wstring utf8_to_wstring (const std::string& str);

// yet another convert wstring to UTF-8 string
std::string narrow( const std::wstring& str );

// parse field descriptor to get length (or type), like I, B, D, [I, [[I, [[Ljava.lang.String; .etc
int parse_field_descriptor(const std::wstring & descriptor);

/*===----------------- Debugging Tool -------------------===*/
class vm_thread;
class InstanceOop;
std::wstring toString(InstanceOop *oop, vm_thread *thread);

/*===-----------------  Constructor ---------------------===*/
// constructors and destructors
template <typename Tp, typename Arg1>
void __constructor(Tp *ptr, const Arg1 & arg1)		// 适配一个参数
{
	::new ((void *)ptr) Tp(arg1);
}

template <typename Tp, typename Arg1, typename Arg2>
void __constructor(Tp *ptr, const Arg1 & arg1, const Arg2 & arg2)		// 适配两个参数
{
	::new ((void *)ptr) Tp(arg1, arg2);
}

template <typename Tp, typename Arg1, typename Arg2>
void __constructor(Tp *ptr, const Arg1 & arg1, Arg2 & arg2)			// 适配两个参数，其中第二个是 非const
{
	::new ((void *)ptr) Tp(arg1, arg2);
}

template <typename Tp, typename Arg1, typename Arg2, typename Arg3>
void __constructor(Tp *ptr, const Arg1 & arg1, const Arg2 & arg2, const Arg3 & arg3)		// 适配三个参数
{
	::new ((void *)ptr) Tp(arg1, arg2, arg3);
}

template <typename Tp, typename ...Args>
void constructor(Tp *ptr, Args &&...args)		// placement new.
{
	__constructor(ptr, std::forward<Args>(args)...);		// 完美转发变长参数
}

template <typename Tp>
void destructor(Tp *ptr)
{
	if (ptr)
		ptr->~Tp();
}





#endif

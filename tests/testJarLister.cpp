#include "jarLister.hpp"
#include <iostream>

// 别忘了链接的时候一定要加上 .o 文件！要不只有头文件，链接不上！

int main()
{
	JarLister jl;
	jl.print();
	assert(jl.find_file(L"java/util/ArrayList$1.class") == 1);
}

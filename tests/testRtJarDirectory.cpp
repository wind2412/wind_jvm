#include <iostream>
#include <jarLister.hpp>

int main()
{
	RtJarDirectory rjd(L"root");
	rjd.add_file(StringSplitter(L"apple/applescript/AppleScriptEngine.class"));
	rjd.add_file(StringSplitter(L"apple/applescript/AppleScriptEngineFactory$1.class"));
	rjd.add_file(StringSplitter(L"apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
	rjd.add_file(StringSplitter(L"haha.class"));
	rjd.add_file(StringSplitter(L"apple/laf/AquaLookAndFeel.class"));
	rjd.add_file(StringSplitter(L"蛤蛤/我的天/你够了.class"));
	rjd.print();

	std::cout << rjd.find_file(StringSplitter(L"apple/applescript/AppleScriptEngineFactory.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter(L"haha.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter(L"apple/laf/AquaLookAndFeel.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter(L"apple/applescript/AppleScriptEngineFactory$.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter(L"蛤蛤/我的天/你够了.class")) << std::endl;
}
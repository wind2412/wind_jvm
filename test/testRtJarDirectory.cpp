#include "../JarLister.h"
#include <iostream>

int main()
{
	RtJarDirectory rjd("root");
	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngine.class"));
	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory$1.class"));
	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
//	rjd.add_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class"));
	rjd.add_file(StringSplitter("haha.class"));
	rjd.add_file(StringSplitter("apple/laf/AquaLookAndFeel.class"));
	rjd.print();

	std::cout << rjd.find_file(StringSplitter("apple/applescript/AppleScriptEngineFactory.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter("haha.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter("apple/laf/AquaLookAndFeel.class")) << std::endl;
	std::cout << rjd.find_file(StringSplitter("apple/applescript/AppleScriptEngineFactory$.class")) << std::endl;
}
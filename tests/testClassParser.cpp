#include <iostream>
#include <fstream>
#include <class_parser.hpp>

using std::cout;
using std::endl;

int main(int argc, char *argv[]) 
{
	std::wcout.imbue(std::locale(""));
	// std::ifstream f("/Users/zhengxiaolin/Documents/cpp/c++11/JNI/C call java functions/JNIDemo.class", std::ios::binary);
	// std::ifstream f("/Users/zhengxiaolin/Documents/cpp/c++11/java_class_parser/Annotations.class", std::ios::binary);
//	std::ifstream f("/Users/zhengxiaolin/Documents/cpp/c++11/java_class_parser/Example2.class", std::ios::binary);
	std::ifstream f("/Users/zhengxiaolin/Documents/cpp/c++11/java_class_parser/BigInteger.class", std::ios::binary);
	if(!f.is_open()) {
		cout << "wrong!" << endl;
		return -1;
 	}
	
	ClassFile cf;
	f >> cf;
	
}
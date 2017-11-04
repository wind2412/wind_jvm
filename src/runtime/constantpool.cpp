/*
 * constantpool.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "runtime/constantpool.hpp"
#include <string>

using std::make_pair;
using std::wstring;

rt_constant_pool::rt_constant_pool(Klass *this_class, int this_class_index, cp_info **bufs, int length, ClassLoader *loader) : loader(loader)
{
	for(int i = 0; i < length; i ++) {
		switch (bufs[i]->tag) {
			case CONSTANT_Class:{
				// TODO: load the class! 并且把运行时的 Klass 引用放进常量池！(自己的话要把 this 放进去！注意是指针。因为我禁止了 Klass 的拷贝，而 any 的实现基于拷贝，且它无法使用引用类型。)
				if (i == this_class_index - 1) {
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(this_class)));		// Klass *
				} else {

				}
				break;
			}
			case CONSTANT_String:{	// TODO: 我认为最后形成的 String 类内部应该封装一个 std::wstring & 才好。这样能够保证常量池唯一！～
				CONSTANT_CS_info* target = (CONSTANT_CS_info*)bufs[i];
				assert(bufs[target->index-1]->tag == CONSTANT_Utf8);
				this->pool.push_back(make_pair(bufs[i]->tag, target->index-1));	// int 索引。因为在 CONSTANT_utf8 中已经保存了一份。所以这里保存一个索引就可以了。
//				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(((CONSTANT_Utf8_info *)bufs[target->index-1])->convert_to_Unicode())));	// wstring
				break;
			}
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:{
				// TODO: make all three! 并且把运行时的 Fieldref Methodref InterfaceMethodref 引用放进常量池！
				CONSTANT_FMI_info* target = (CONSTANT_FMI_info*)bufs[i];
				if (target->tag == CONSTANT_Fieldref)
					printf("(DEBUG) #%4d = Fieldref %12s #%d.#%d\n", i+1, "", target->class_index, target->name_and_type_index);
				else if (target->tag == CONSTANT_Methodref)
					printf("(DEBUG) #%4d = Methodref %11s #%d.#%d\n", i+1, "", target->class_index, target->name_and_type_index);
				else
					printf("(DEBUG) #%4d = InterfaceMethodref %2s #%d.#%d\n", i+1, "", target->class_index, target->name_and_type_index);
				break;
			}
			case CONSTANT_Integer:{
				CONSTANT_Integer_info* target = (CONSTANT_Integer_info*)bufs[i];
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target->get_value())));		// int
				break;
			}
			case CONSTANT_Float:{
				CONSTANT_Float_info* target = (CONSTANT_Float_info*)bufs[i];
				// TODO: 不知道转回来再转回去会不会有精度问题。
				// TODO: 别忘了在外边验证 FLOAT_NAN 等共计 3 个问题。
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target->get_value())));		// float
				break;
			}
			case CONSTANT_Long:{
				CONSTANT_Long_info* target = (CONSTANT_Long_info*)bufs[i];
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target->get_value())));		// long
				i ++;
				this->pool.push_back(make_pair(-1, boost::any()));									// 由于有一位占位符，因此 empty。
				break;
			}
			case CONSTANT_Double:{
				// TODO: 不知道转回来再转回去会不会有精度问题。
				// TODO: 别忘了在外边验证 FLOAT_NAN 等共计 3 个问题。
				CONSTANT_Double_info* target = (CONSTANT_Double_info*)bufs[i];
				double result = target->get_value();
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target->get_value())));		// double
				i ++;
				this->pool.push_back(make_pair(-1, boost::any()));									// 由于有一位占位符，因此 empty。
				break;
			}
			case CONSTANT_NameAndType:{
				// TODO: 把 wstring 和 Klass 组成一个 pair 放进常量池！
				CONSTANT_NameAndType_info* target = (CONSTANT_NameAndType_info*)bufs[i];
				printf("(DEBUG) #%4d = NameAndType %9s #%d.#%d\n", i+1, "", target->name_index, target->descriptor_index);
				break;
			}
			case CONSTANT_Utf8:{
				CONSTANT_Utf8_info* target = (CONSTANT_Utf8_info*)bufs[i];
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(new wstring(target->convert_to_Unicode()))));		// wstring * 类型。这样能够保证常量唯一。
				break;
			}
			default:{
				// TODO: finish these three below:
				std::cerr << "didn't finish MethodHandle, MethodType and InvokeDynamic!!" << std::endl;
				assert(false);
			}
//			case CONSTANT_MethodHandle:{		// TODO !!!!!!!!!!!!
//
//			}
//			case CONSTANT_MethodType:
//			case CONSTANT_InvokeDynamic:;
		}
	}
}

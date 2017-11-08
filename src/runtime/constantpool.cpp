/*
 * constantpool.cpp
 *
 *  Created on: 2017年11月2日
 *      Author: zhengxiaolin
 */

#include "runtime/constantpool.hpp"
#include "classloader.hpp"
#include "runtime/klass.hpp"
#include "runtime/field.hpp"
#include <string>

using std::make_pair;
using std::wstring;
using std::make_shared;

//int counter = 0;

shared_ptr<Klass> rt_constant_pool::if_didnt_load_then_load(ClassLoader *loader, const wstring & name)
{
	if (loader == nullptr) {
		return BootStrapClassLoader::get_bootstrap().loadClass(name);
	} else {
		return loader->loadClass(name);
	}
}

// 运行时常量池解析。包括解析类、字段以及方法属性。方法字节码的解析留到最后进行。
rt_constant_pool::rt_constant_pool(shared_ptr<InstanceKlass> this_class, ClassLoader *loader, const ClassFile & cf) : loader(loader)
{
	int this_class_index = cf.this_class;
	cp_info **bufs = cf.constant_pool;
	int length = cf.constant_pool_count;

	for(int i = 0; i < length-1; i ++) {		// 别忘了 -1 啊！！！！
		switch (bufs[i]->tag) {
			case CONSTANT_Class:{
				if (i == this_class_index - 1) {
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(shared_ptr<Klass>(this_class))));		// shared_ptr<Klass>，不过其实真正的是 shared_ptr<InstanceKlass>.
				} else {
					// get should-be-loaded class name
					CONSTANT_CS_info* target = (CONSTANT_CS_info*)bufs[i];
					assert(bufs[target->index-1]->tag == CONSTANT_Utf8);
					wstring name = ((CONSTANT_Utf8_info *)bufs[target->index-1])->convert_to_Unicode();	// e.g. java/lang/Object
					// load the class
					std::wcout << "load class ===> " << "<" << name << ">" << std::endl;
					shared_ptr<Klass> new_class = if_didnt_load_then_load(loader, name);	// TODO: 这里可能得到数组类！需要额外判断一下！
					assert(new_class != nullptr);
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(shared_ptr<Klass>(new_class))));			// shared_ptr<Klass> ，不过可能可以是 InstanceKlass 或者 TypeArrayKlass 或者 ObjArrayKlass......
																													// TODO: 所以必须要有一种方法来判断型别......明天试试 traits??????
				}
				break;
			}
			case CONSTANT_String:{	// TODO: 我认为最后形成的 String 类内部应该封装一个 std::wstring & 才好。这样能够保证常量池唯一！～
				CONSTANT_CS_info* target = (CONSTANT_CS_info*)bufs[i];
				assert(bufs[target->index-1]->tag == CONSTANT_Utf8);
				this->pool.push_back(make_pair(target->tag, boost::any((int)target->index)));			// int 索引。因为在 CONSTANT_utf8 中已经保存了一份。所以这里保存一个索引就可以了。
//				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(((CONSTANT_Utf8_info *)bufs[target->index-1])->convert_to_Unicode())));	// wstring
				break;
			}
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:{
				// TODO: make all three! 并且把运行时的 Fieldref Methodref InterfaceMethodref 引用放进常量池！
				CONSTANT_FMI_info* target = (CONSTANT_FMI_info*)bufs[i];
				assert(bufs[target->class_index-1]->tag == CONSTANT_Class);
				assert(bufs[target->name_and_type_index-1]->tag == CONSTANT_NameAndType);
				// get class name
				wstring class_name = ((CONSTANT_Utf8_info *)bufs[((CONSTANT_CS_info *)bufs[target->class_index-1])->index-1])->convert_to_Unicode();
				// load class
				shared_ptr<Klass> new_class = std::static_pointer_cast<Klass>(if_didnt_load_then_load(loader, class_name));		// [—— 这里不可能得到数组类]。错！！卧槽...... 这里调试了一天...... 太武断了！比如这里，就要从 一个数组类中去寻找 java.lang.Object::clone 方法！！！
				assert(new_class != nullptr);
				// get field/method/interface_method name
				auto name_type_ptr = (CONSTANT_NameAndType_info *)bufs[target->name_and_type_index-1];
				wstring name = ((CONSTANT_Utf8_info *)bufs[name_type_ptr->name_index-1])->convert_to_Unicode();
				wstring descriptor = ((CONSTANT_Utf8_info *)bufs[name_type_ptr->descriptor_index-1])->convert_to_Unicode();

				// Methodref 和 InterfaceMethodref 的信息丢失了。他俩混杂在一块了。不知道会有什么样的后果？其实也没丢失。在 this->pool 的 pair.second.first 里边存着（逃
				// 上边问题的解决：因为 解析 的方式不同，看下文方法的实现也不同。一个调用 get_class_method，另一个调用 get_interface_method。因此可以解除上边的疑惑。
				if (target->tag == CONSTANT_Fieldref) {
					std::wcout << "find field ===> " << "<" << class_name << ">" << name + L":" + descriptor << std::endl;
					assert(new_class->get_type() == ClassType::InstanceClass);
					shared_ptr<Field_info> target = std::static_pointer_cast<InstanceKlass>(new_class)->get_field(name + L":" + descriptor);		// 这里才是不可能得到数组类。因为数组类 和 Object 都没有 field 把。所以可以直接强转了。
					assert(target != nullptr);		// TODO: 在这里我的程序正确性还需要验证。正常情况下应该抛出异常。不过我默认所有的 class 文件全是 **完全正确** 的，因此没有做 verify。这些细枝末节留到全写完之后回来在增加吧。
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target)));				// shared_ptr<Field_info>
				} else if (target->tag == CONSTANT_Methodref) {
					std::wcout << "find class method ===> " << "<" << class_name << ">" << name + L":" + descriptor << std::endl;
//	counter ++ ;
//	std::wcout << __FILE__ << " " << __LINE__ << " " << counter << std::endl;
//	assert(new_class->get_methods().size() < 500);
//	std::wcout << __FILE__ << " " << __LINE__ << std::endl;
//	std::wcout << new_class->get_name() << " " << new_class->get_methods().size() << std::endl;
//	for (auto ll : new_class->get_methods()) {
//		std::wcout << ll.first << std::endl;
//	}
					shared_ptr<Method> target;
					if (new_class->get_type() == ClassType::ObjArrayClass || new_class->get_type() == ClassType::TypeArrayClass) {
						target = std::static_pointer_cast<ArrayKlass>(new_class)->get_class_method(name + L":" + descriptor);	// 这里可能是 数组类 和 普通类。需要判断才行。
					} else if (new_class->get_type() == ClassType::InstanceClass){
						target = std::static_pointer_cast<InstanceKlass>(new_class)->get_class_method(name + L":" + descriptor);
					} else {
						std::cerr << "only support ArrayKlass and InstanceKlass now!" << std::endl;
						assert(false);
					}
					assert(target != nullptr);
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target)));				// shared_ptr<Method>
				} else {	// InterfaceMethodref
					std::wcout << "find interface method ===> " << "<" << class_name << ">" << name + L":" + descriptor << std::endl;
					assert(new_class->get_type() == ClassType::InstanceClass);
					shared_ptr<Method> target = std::static_pointer_cast<InstanceKlass>(new_class)->get_interface_method(name + L":" + descriptor);		// 这里应该只有可能是普通类吧。应该不是未实现的接口方法。因为 java.lang.Object 是一个 Class。所以选择直接强转了。
					assert(target != nullptr);
					this->pool.push_back(make_pair(bufs[i]->tag, boost::any(target)));				// shared_ptr<Method>
				}
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
				CONSTANT_NameAndType_info* target = (CONSTANT_NameAndType_info*)bufs[i];
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(make_pair((int)target->name_index, (int)target->descriptor_index))));	// pair<u2, u2> 索引。
				break;
			}
			case CONSTANT_Utf8:{
				CONSTANT_Utf8_info* target = (CONSTANT_Utf8_info*)bufs[i];
				this->pool.push_back(make_pair(bufs[i]->tag, boost::any(make_shared<wstring>(target->convert_to_Unicode()))));		// shared_ptr<wstring> 类型。这样能够保证常量唯一。
				break;
			}
			case CONSTANT_MethodHandle:{	// TODO:!!

				break;
			}
			case CONSTANT_MethodType:{	// TODO:!!

				break;
			}
			case CONSTANT_InvokeDynamic:{	// TODO:!!

				break;
			}
			default:{
				std::cerr << "wrong! tag == " << (int)bufs[i]->tag << std::endl;
				std::cerr << "didn't finish MethodHandle, MethodType and InvokeDynamic!!" << std::endl;
				assert(false);
			}
		}
	}
}

void rt_constant_pool::print_debug()
{
	std::cout << "rt_pool: total " << this->pool.size() << std::endl;
	int counter = 1;			// [p.s.] this is 1...
	bool next_jump = false;	// for Long and Double take 2 constant_pool positions.
	for (auto iter : this->pool) {
		if (next_jump) {
			next_jump = false;
			counter ++;
			continue;
		}
		std::wcout << "  #" << counter++ << " = ";
		switch (iter.first) {
			case CONSTANT_Class:{
				std::wcout << "Class              ===> ";
//				std::cout << &*boost::any_cast<shared_ptr<Klass>>(iter.second) << std::endl;
				std::wcout << boost::any_cast<shared_ptr<Klass>>(iter.second)->get_name() << std::endl;
				break;
			}
			case CONSTANT_String:{
				std::wcout << "String             ===> ";
				assert(this->pool[boost::any_cast<int>(iter.second)-1].first == CONSTANT_Utf8);		// 别忘了 -1......QAQQAQ
				std::wcout << *boost::any_cast<shared_ptr<wstring>>(this->pool[boost::any_cast<int>(iter.second)-1].second) << std::endl;
				break;
			}
			case CONSTANT_Fieldref:{
				std::wcout << "Fieldref           ===> ";
				boost::any_cast<shared_ptr<Field_info>>(iter.second)->print();
				std::wcout << std::endl;
				break;
			}
			case CONSTANT_Methodref:{
				std::wcout << "Methodref          ===> ";

				break;
			}
			case CONSTANT_InterfaceMethodref:{
				std::wcout << "InterfaceMethodref ===> ";

				break;
			}
			case CONSTANT_Integer:{
				std::wcout << "Integer            ===> ";

				break;
			}
			case CONSTANT_Float:{
				std::wcout << "Float              ===> ";

				break;
			}
			case CONSTANT_Long:{
				std::wcout << "Long               ===> ";

				next_jump = true;
				break;
			}
			case CONSTANT_Double:{
				std::wcout << "Double             ===> ";

				next_jump = true;
				break;
			}
			case CONSTANT_NameAndType:{
				std::wcout << "NameAndType        ===> ";

				break;
			}
			case CONSTANT_Utf8:{
				std::wcout << "Utf8               ===> ";

				break;
			}
			case CONSTANT_MethodHandle:{	// TODO:!!
				std::wcout << "MethodHandle       ===> ";

				break;
			}
			case CONSTANT_MethodType:{	// TODO:!!
				std::wcout << "MethodType         ===> ";

				break;
			}
			case CONSTANT_InvokeDynamic:{	// TODO:!!
				std::wcout << "InvokeDynamic      ===> ";

				break;
			}
			default:{
				std::cerr << "can't get here!" << std::endl;
				assert(false);
			}
		}
	}
}

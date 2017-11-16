/*
 * bytecodeEngine.cpp
 *
 *  Created on: 2017年11月12日
 *      Author: zhengxiaolin
 */

#include "runtime/bytecodeEngine.hpp"
#include "wind_jvm.hpp"
#include "runtime/method.hpp"
#include "runtime/constantpool.hpp"
#include <memory>
#include <functional>
#include <boost/any.hpp>

using std::list;
using std::function;
using std::shared_ptr;

/*===----------- StackFrame --------------===*/
StackFrame::StackFrame(Oop *_this, shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev, const list<uint64_t> & list) : method(method), return_pc(return_pc), prev(prev) {	// va_args is: Method's argument. 所有的变长参数的类型全是没有类型的 uint64_t。因此，在**执行 code**的时候才会有类型提升～
	// TODO: set this pointer...
	localVariableTable.insert(localVariableTable.end(), list.begin(), list.end());
}

void StackFrame::clear_all() {					// used with `is_valid()`. if invalid, clear all to reuse this frame.
	this->valid_frame = true;
	stack<uint64_t>().swap(op_stack);				// empty. [Effective STL]
	vector<uint64_t>().swap(localVariableTable);
	method = nullptr;
	return_pc = nullptr;
	// prev not change.
}

/*===------------ BytecodeEngine ---------------===*/

vector<Type> BytecodeEngine::parse_arg_list(const wstring & descriptor)
{
	vector<Type> arg_list;
	for (int i = 1; i < descriptor.size(); i ++) {		// ignore the first L'('.
		// define
		function<void(wchar_t)> recursive_arg = [&arg_list, &descriptor, &recursive_arg, &i](wchar_t cur) {
			switch (descriptor[i]) {
				case L'Z':{
					arg_list.push_back(Type::BOOLEAN);
					break;
				}
				case L'B':{
					arg_list.push_back(Type::BYTE);
					break;
				}
				case L'C':{
					arg_list.push_back(Type::CHAR);
					break;
				}
				case L'S':{
					arg_list.push_back(Type::SHORT);
					break;
				}
				case L'I':{
					arg_list.push_back(Type::INT);
					break;
				}
				case L'F':{
					arg_list.push_back(Type::FLOAT);
					break;
				}
				case L'J':{
					arg_list.push_back(Type::LONG);
					break;
				}
				case L'D':{
					arg_list.push_back(Type::DOUBLE);
					break;
				}
				case L'L':{
					arg_list.push_back(Type::OBJECT);
					while(descriptor[i] != L';') {
						i ++;
					}
					break;
				}
				case L'[':{
					arg_list.push_back(Type::ARRAY);
					while(descriptor[i] == L'[') {
						i ++;
					}
					recursive_arg(descriptor[i]);	// will push_back a wrong answer
					arg_list.pop_back();					// will pop_back it.
					break;
				}
			}
		};
		// execute
		if (descriptor[i] == L')') {
			break;
		}
		recursive_arg(descriptor[i]);
	}
	return arg_list;
}

void BytecodeEngine::initial_clinit(shared_ptr<InstanceKlass> new_klass, wind_jvm & jvm)
{
	if (new_klass->get_state() == Klass::KlassState::NotInitialized) {
		// recursively initialize its parent first !!!!! So java.lang.Object must be the first !!!
		if (new_klass->get_parent() != nullptr)	// prevent this_klass is the java.lang.Object.
			BytecodeEngine::initial_clinit(std::static_pointer_cast<InstanceKlass>(new_klass->get_parent()), jvm);
		// then initialize this_klass.
		std::wcout << "(DEBUG) " << new_klass->get_name() << "::<clinit>" << std::endl;
		shared_ptr<Method> clinit = new_klass->get_this_class_method(L"<clinit>:()V");		// **IMPORTANT** only search in this_class for `<clinit>` !!!
		if (clinit != nullptr) {		// TODO: 这里 clinit 不知道会如何执行。
			new_klass->set_state(Klass::KlassState::Initializing);		// important.
			jvm.add_frame_and_execute(clinit, {});		// no return value
		} else {
			std::cout << "(DEBUG) no <clinit>." << std::endl;
		}
		new_klass->set_state(Klass::KlassState::Initialized);
	}
}

Oop * BytecodeEngine::execute(wind_jvm & jvm, StackFrame & cur_frame) {		// 卧槽......vector 由于扩容，会导致内部的引用全部失效...... 改成 list 吧......却是忽略了这点。

	shared_ptr<Method> method = cur_frame.method;
	uint32_t code_length = method->get_code()->code_length;
	stack<uint64_t> & op_stack = cur_frame.op_stack;
	vector<uint64_t> & localVariableTable = cur_frame.localVariableTable;
	uint8_t *code_begin = method->get_code()->code;
	shared_ptr<InstanceKlass> klass = method->get_klass();
	rt_constant_pool & rt_pool = *klass->get_rtpool();

	uint8_t *backup_pc = jvm.pc;
	uint8_t * & pc = jvm.pc;
	pc = code_begin;

	while (pc < code_begin + code_length) {
		std::wcout << L"(DEBUG) <bytecode> " << klass->get_name() << "::" << method->get_name() << ":" << method->get_descriptor() << " --> " << utf8_to_wstring(bccode_map[*pc].first) << std::endl;
		int occupied = bccode_map[*pc].second + 1;		// the bytecode takes how many bytes.(include itself)		// TODO: tableswitch, lookupswitch, wide is NEGATIVE!!!
		switch(*pc) {

			case 0x01:{		// aconst_null
				op_stack.push(0);		// TODO: 我只压入了 0.
#ifdef DEBUG
	std::cout << "(DEBUG) push null on stack." << std::endl;
#endif
				break;
			}

			case 0x03:{		// iconst_0
				op_stack.push(0);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 0 on stack." << std::endl;
#endif
				break;
			}
			case 0x04:{		// iconst_1
				op_stack.push(1);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 1 on stack." << std::endl;
#endif
				break;
			}
			case 0x05:{		// iconst_2
				op_stack.push(2);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 2 on stack." << std::endl;
#endif
				break;
			}
			case 0x06:{		// iconst_3
				op_stack.push(3);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 3 on stack." << std::endl;
#endif
				break;
			}
			case 0x07:{		// iconst_4
				op_stack.push(4);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 4 on stack." << std::endl;
#endif
				break;
			}
			case 0x08:{		// iconst_5
				op_stack.push(5);
#ifdef DEBUG
	std::cout << "(DEBUG) push int 5 on stack." << std::endl;
#endif
				break;
			}

			case 0x12:{		// ldc
				int rtpool_index = pc[1];
				if (rt_pool[rtpool_index-1].first == CONSTANT_Integer) {
					int value = boost::any_cast<int>(rt_pool[rtpool_index-1].second);
					op_stack.push(value);
#ifdef DEBUG
	std::cout << "(DEBUG) push int: "<< value << "on stack." << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_Float) {
					float value = boost::any_cast<float>(rt_pool[rtpool_index-1].second);
					op_stack.push(value);
#ifdef DEBUG
	std::cout << "(DEBUG) push float: "<< value << "on stack." << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_String) {
					InstanceOop *stringoop = (InstanceOop *)boost::any_cast<Oop *>(rt_pool[rtpool_index-1].second);
					op_stack.push((uint64_t)stringoop);
#ifdef DEBUG
	// for string:
	uint64_t result;
	bool temp = stringoop->get_field_value(L"value:[C", &result);
	assert(temp == true);
	std::cout << "string length: " << ((TypeArrayOop *)result)->get_length() << std::endl;
	std::cout << "the string is: --> ";
	for (int pos = 0; pos < ((TypeArrayOop *)result)->get_length(); pos ++) {
		std::wcout << wchar_t(((CharOop *)(*(TypeArrayOop *)result)[pos])->value);
	}
	std::wcout.clear();
	std::wcout << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_Class) {
					auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);
					assert(klass->get_mirror() != nullptr);
					op_stack.push((uint64_t)klass->get_mirror());		// push into [Oop*] type.
#ifdef DEBUG
	std::wcout << "(DEBUG) push class: "<< klass->get_name() << "'s mirror "<< "on stack." << std::endl;
#endif
				} else {
					std::cerr << "can't get here!" << std::endl;
					assert(false);
				}
				break;
			}



			case 0x1a:{		// iload_0
				op_stack.push(localVariableTable[0]);
#ifdef DEBUG
	std::cout << "(DEBUG) push localVariableTable int: "<< (int)localVariableTable[0] << "on stack." << std::endl;
#endif
				break;
			}
			case 0x1b:{		// iload_1
				op_stack.push(localVariableTable[1]);
#ifdef DEBUG
	std::cout << "(DEBUG) push localVariableTable int: "<< (int)localVariableTable[0] << "on stack." << std::endl;
#endif
				break;
			}
			case 0x1c:{		// iload_2
				op_stack.push(localVariableTable[2]);
#ifdef DEBUG
	std::cout << "(DEBUG) push localVariableTable int: "<< (int)localVariableTable[1] << "on stack." << std::endl;
#endif
				break;
			}
			case 0x1d:{		// iload_3
				op_stack.push(localVariableTable[3]);
#ifdef DEBUG
	std::cout << "(DEBUG) push localVariableTable int: "<< (int)localVariableTable[2] << "on stack." << std::endl;
#endif
				break;
			}

			case 0x2a:{		// aload_0
				op_stack.push(localVariableTable[0]);
#ifdef DEBUG
	std::wcout << "(DEBUG) push localVariableTable ref: "<< ((Oop *)localVariableTable[0])->get_klass()->get_name() << "'s Oop: address: 0x" << std::hex << localVariableTable[0] << " on stack." << std::endl;
#endif
				break;
			}
			case 0x2b:{		// aload_1
				op_stack.push(localVariableTable[1]);
#ifdef DEBUG
	std::wcout << "(DEBUG) push localVariableTable ref: "<< ((Oop *)localVariableTable[1])->get_klass()->get_name() << "'s Oop: address: 0x" << std::hex << localVariableTable[1] << " on stack." << std::endl;
#endif
				break;
			}
			case 0x2c:{		// aload_2
				op_stack.push(localVariableTable[2]);
#ifdef DEBUG
	std::wcout << "(DEBUG) push localVariableTable ref: "<< ((Oop *)localVariableTable[2])->get_klass()->get_name() << "'s Oop: address: 0x" << std::hex << localVariableTable[2] << " on stack." << std::endl;
#endif
				break;
			}
			case 0x2d:{		// aload_3
				op_stack.push(localVariableTable[3]);
#ifdef DEBUG
	std::wcout << "(DEBUG) push localVariableTable ref: "<< ((Oop *)localVariableTable[3])->get_klass()->get_name() << "'s Oop: address: 0x" << std::hex << localVariableTable[3] << " on stack." << std::endl;
#endif
				break;
			}

			case 0x57:{		// pop
				op_stack.pop();
#ifdef DEBUG
	std::wcout << "(DEBUG) only pop from stack." << std::endl;
#endif
				break;
			}

			case 0x59:{		// dup
				op_stack.push(op_stack.top());
#ifdef DEBUG
	std::wcout << "(DEBUG) only dup from stack." << std::endl;
#endif
				break;
			}


			case 0x60:{		// iadd
				int val1 = op_stack.top(); op_stack.pop();
				int val2 = op_stack.top(); op_stack.pop();
				op_stack.push(val1 + val2);
#ifdef DEBUG
	std::cout << "(DEBUG) add int value from stack: "<< val1 << "+" << val2 << " and put " << (val1+val2) << " on stack." << std::endl;
#endif
				break;
			}


			case 0xac:{		// ireturn
				// TODO: monitor...
				jvm.pc = backup_pc;
#ifdef DEBUG
	std::cout << "(DEBUG) return an int value from stack: "<< (int)op_stack.top() << std::endl;
#endif
				return new IntOop(op_stack.top());	// boolean, short, char, int
			}



			case 0xb1:{		// return
				// TODO: monitor...
				jvm.pc = backup_pc;
#ifdef DEBUG
	std::cout << "(DEBUG) only return." << std::endl;
#endif
				return nullptr;
			}
			case 0xb2:{		// getStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_field->get_klass();
				initial_clinit(new_klass, jvm);
				// parse the field to RUNTIME!!
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				if (new_field->get_type() == Type::OBJECT) {
					// TODO: <clinit> of the Field object oop......
					assert(new_field->get_type_klass() != nullptr);
					initial_clinit(std::static_pointer_cast<InstanceKlass>(new_field->get_type_klass()), jvm);
				}
				// get the [static Field] value and save to the stack top
				uint64_t new_top;
				bool temp = new_klass->get_static_field_value(new_field, &new_top);
				assert(temp == true);
				op_stack.push(new_top);
#ifdef DEBUG
	std::wcout << "(DEBUG) get a static value (unknown value type): " << new_top << " from <class>: " << new_klass->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " on to the stack." << std::endl;
#endif
				break;
			}
			case 0xb3:{		// putStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_field->get_klass();
				initial_clinit(new_klass, jvm);
				// parse the field to RUNTIME!!
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				if (new_field->get_type() == Type::OBJECT) {
					// TODO: <clinit> of the Field object oop......
					assert(new_field->get_type_klass() != nullptr);
					initial_clinit(std::static_pointer_cast<InstanceKlass>(new_field->get_type_klass()), jvm);
				}
				// get the stack top and save to the [static Field]
				uint64_t top = op_stack.top();	op_stack.pop();
				new_klass->set_static_field_value(new_field, top);
#ifdef DEBUG
	std::wcout << "(DEBUG) put a static value (unknown value type): " << top << " from stack, to <class>: " << new_klass->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " and override." << std::endl;
#endif
				break;
			}

			case 0xb5:{		// putField
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// TODO: $2.8.3 的 FP_strict 浮点数转换！
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				uint64_t new_value = op_stack.top();	op_stack.pop();
				Oop *ref = (Oop *)op_stack.top();	op_stack.pop();
				assert(ref->get_klass()->get_type() == ClassType::InstanceClass);		// bug !!! 有可能是没有把 this 指针放到上边。
//				std::wcout << ref->get_klass()->get_name() << " " << new_field->get_klass()->get_name() << std::endl;
//				assert(ref->get_klass() == new_field->get_klass());	// 不正确。因为左边可能是右边的子类。
				((InstanceOop *)ref)->set_field_value(new_field, new_value);
#ifdef DEBUG
	std::wcout << "(DEBUG) put a non-static value (unknown value type): " << new_value << " from stack, to <class>: " << ref->get_klass()->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " and override." << std::endl;
#endif
				break;
			}

			case 0xb7:		// invokeSpecial
			case 0xb8:{		// invokeStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Methodref);
				auto new_method = boost::any_cast<shared_ptr<Method>>(rt_pool[rtpool_index-1].second);
				if (*pc == 0xb8) {
					assert(new_method->is_static() && !new_method->is_abstract());
				} else if (*pc == 0xb7) {
					// TODO: 可以有限制条件。
				}
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_method->get_klass();
				initial_clinit(new_klass, jvm);
				std::wcout << "(DEBUG) " << new_klass->get_name() << "::" << new_method->get_name() << ":" << new_method->get_descriptor() << std::endl;	// msg
				if (new_method->is_synchronized()) {
					// TODO: synchronized !!!!!!
					std::cerr << "can't suppose synchronized now..." << std::endl;
//					assert(false);
				}
				if (new_method->is_native()) {
					// TODO: native
					std::cerr << "can't suppose native now..." << std::endl;
//					assert(false);
				} else {
					int size = BytecodeEngine::parse_arg_list(new_method->get_descriptor()).size();
					if (*pc == 0xb7) {
						size ++;		// invokeSpecial 必须加入一个 this 指针！除了 invokeStatic 之外的所有指令都要加上 this 指针！！！ ********* important ！！！！！
									// this 指针会被自动放到 op_stack 上！所以，从 op_stack 上多读一个就 ok ！！
					}
					std::cout << "arg size: " << size << "; op_stack size: " << op_stack.size() << std::endl;	// delete
					// TODO: 参数应该是倒着入栈的吧...?
					list<uint64_t> arg_list;
					assert(op_stack.size() >= size);
					while (size > 0) {
						arg_list.push_front(op_stack.top());
						op_stack.pop();
						size --;
					}
#ifdef DEBUG
	if (*pc == 0xb7)
		std::wcout << "(DEBUG) invoke a method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":(this)"<< new_method->get_descriptor() << std::endl;
	else if (*pc == 0xb8)
		std::wcout << "(DEBUG) invoke a method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":"<< new_method->get_descriptor() << std::endl;
#endif
					Oop *result = jvm.add_frame_and_execute(new_method, arg_list);
					if (!new_method->is_void()) {
						op_stack.push((uint64_t)result);
					}
				}
				break;
			}

			case 0xbb:{		// new // 仅仅分配了内存！
				// TODO: 这里不是很明白。规范中写：new 的后边可能会跟上一个常量池索引，这个索引指向类或接口......接口是什么鬼???? 还能被实例化吗 ???
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Class);
				auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);
				assert(klass->get_type() == ClassType::InstanceClass);		// TODO: 并不知道对不对...猜的.
				auto real_klass = std::static_pointer_cast<InstanceKlass>(klass);
				// if didnt init then init
				initial_clinit(real_klass, jvm);
				auto oop = real_klass->new_instance();
				op_stack.push((uint64_t)oop);
#ifdef DEBUG
	std::wcout << "(DEBUG) new an object (only alloc memory): <class>: " << klass->get_name() << std::endl;
#endif
				break;
			}

			case 0xbd:{		// anewarray		// 创建引用(对象)的[一维]数组。
				/**
				 * java 的数组十分神奇。在这里需要有所解释。
				 * String[][] x = new String[2][3]; 调用的是 mulanewarray. 这样初始化出来的 ArrayOop 是 [二维] 的。即 dimension == 2.
				 * String[][] x = new String[2][]; x[0] = new String[2]; x[1] = new String[3]; 调用的是 anewarray. 这样初始化出来的 ArrayOop [全部] 都是 [一维] 的。即 dimension == 1.
				 * 虽然句柄的表示 String[][] 都是这个格式，但是 dimension 不同！前者，x 仅仅是一个 二维的 ArrayOop，element 是 java.lang.String。
				 * 而后者的 x 是一个 一维的 ArrayOop，element 是 [Ljava.lang.String;。
				 * 实现时千万要注意。
				 * 而且。别忘了可以有 String[] x = new String[0]。
				 * ** 产生这种表示法的根本原因在于：jvm 根本就不关心句柄是怎么表示的。它只关心的是真正的对象。句柄的表示是由编译器来关注并 parse 的！！**
				 */
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				int length = op_stack.top();	op_stack.pop();
				if (length < 0) {	// TODO: 最后要全部换成异常！
					std::cerr << "array length can't be negative!!" << std::endl;
					assert(false);
				}
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Class);
				auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);
				if (klass->get_type() == ClassType::InstanceClass) {			// java/lang/Class
					auto real_klass = std::static_pointer_cast<InstanceKlass>(klass);
					// 由于仅仅是创建一维数组：所以仅仅需要把高一维的数组类加载就好。
					if (real_klass->get_classloader() == nullptr) {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[L" + real_klass->get_name() + L";"));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push((uint64_t)arr_klass->new_instance(length));
					} else {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass->get_classloader()->loadClass(L"[L" + real_klass->get_name() + L";"));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push((uint64_t)arr_klass->new_instance(length));
					}
				} else if (klass->get_type() == ClassType::ObjArrayClass) {	// [Ljava/lang/Class
					auto real_klass = std::static_pointer_cast<ObjArrayKlass>(klass);
					// 创建数组的数组。不过也和 if 中的逻辑相同。
					// 不过由于数组类没有设置 classloader，需要从 element 中去找。
					if (real_klass->get_element_type()->get_classloader() == nullptr) {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[" + real_klass->get_name()));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push((uint64_t)arr_klass->new_instance(length));
					} else {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass->get_element_type()->get_classloader()->loadClass(L"[" + real_klass->get_name()));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push((uint64_t)arr_klass->new_instance(length));
					}
				} else {
					assert(false);
				}
#ifdef DEBUG
	std::wcout << "(DEBUG) new an array[] of class: <class>: " << klass->get_name() << std::endl;
#endif
				break;
			}

			case 0xbf:{		// athrow

				assert(false);

				break;
			}

			case 0xc7:{		// ifnonnull
				int branch_pc = ((pc[1] << 8) | pc[2]);
				uint64_t ref_value = op_stack.top();	op_stack.pop();
				if (ref_value != 0) {	// if not null, jump to the branch_pc.
					pc = (code_begin) + branch_pc;
					pc -= occupied;		// 因为最后设置了 pc += occupied 这个强制增加，因而这里强制减少。
				} else {		// if null, go next.
					// do nothing
				}
				break;
			}


			default:
				std::cerr << "doesn't support bytecode " << bccode_map[*pc].first << " now..." << std::endl;
				assert(false);
		}
		pc += occupied;
	}

	return nullptr;
}


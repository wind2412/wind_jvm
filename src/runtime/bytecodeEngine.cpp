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
		std::wcout << L"(DEBUG) " << klass->get_name() << "::" << method->get_name() << ":" << method->get_descriptor() << " --> " << utf8_to_wstring(bccode_map[*pc].first) << std::endl;
		int occupied = bccode_map[*pc].second + 1;		// the bytecode takes how many bytes.(include itself)		// TODO: tableswitch, lookupswitch, wide is NEGATIVE!!!
		switch(*pc) {

			case 0x03:{		// iconst_0
				op_stack.push(0);
				break;
			}
			case 0x04:{		// iconst_1
				op_stack.push(1);
				break;
			}
			case 0x05:{		// iconst_2
				op_stack.push(2);
				break;
			}
			case 0x06:{		// iconst_3
				op_stack.push(3);
				break;
			}
			case 0x07:{		// iconst_4
				op_stack.push(4);
				break;
			}
			case 0x08:{		// iconst_5
				op_stack.push(5);
				break;
			}


			case 0x1a:{		// iload_0
				op_stack.push(localVariableTable[0]);
				break;
			}
			case 0x1b:{		// iload_1
				op_stack.push(localVariableTable[1]);
				break;
			}
			case 0x1c:{		// iload_2
				op_stack.push(localVariableTable[2]);
				break;
			}
			case 0x1d:{		// iload_3
				op_stack.push(localVariableTable[3]);
				break;
			}

			case 0x57:{		// pop
				op_stack.pop();
				break;
			}

			case 0x60:{		// iadd
				int val1 = op_stack.top(); op_stack.pop();
				int val2 = op_stack.top(); op_stack.pop();
				op_stack.push(val1 + val2);
				break;
			}


			case 0xac:{		// ireturn
				// TODO: monitor...
				jvm.pc = backup_pc;
				return new BasicTypeOop(Type::INT, op_stack.top());	// boolean, short, char, int
			}



			case 0xb1:{		// return
				// TODO: monitor...
				jvm.pc = backup_pc;
				return nullptr;
			}

			case 0xb2:{		// getStatic



				break;
			}

			case 0xb8:{		// invokeStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Methodref);
				auto new_method = boost::any_cast<shared_ptr<Method>>(rt_pool[rtpool_index-1].second);
				assert(new_method->is_static() && !new_method->is_abstract());
				if (!klass->is_initialized()) {
					// initialize a class... <clinit>
					std::wcout << "(DEBUG) " << klass->get_name() << "::<clinit>" << std::endl;		// msg
					shared_ptr<Method> clinit = klass->get_class_method(klass->get_name() + L":<clinit>");
					if (clinit != nullptr) {
						jvm.add_frame_and_execute(clinit, {});		// no return value
					}		// TODO: 这里 clinit 不知道会如何执行。
					klass->set_initialized();
				}
				std::wcout << "(DEBUG) " << klass->get_name() << "::" << new_method->get_name() << ":" << new_method->get_descriptor() << std::endl;	// msg
				// TODO: synchronized !!!!!!
				if (new_method->is_synchronized()) {
					std::cerr << "can't suppose synchronized now..." << std::endl;
					assert(false);
				}
				if (new_method->is_native()) {
					// TODO: native
					std::cerr << "can't suppose native now..." << std::endl;
				} else {
					int size = BytecodeEngine::parse_arg_list(new_method->get_descriptor()).size();
					std::cout << "arg size: " << size << "; op_stack size: " << op_stack.size() << std::endl;	// delete
					// TODO: 参数应该是倒着入栈的吧...?
					list<uint64_t> arg_list;
					assert(op_stack.size() >= size);
					while (size > 0) {
						arg_list.push_front(op_stack.top());
						op_stack.pop();
						size --;
					}
					Oop *result = jvm.add_frame_and_execute(new_method, arg_list);
					if (!new_method->is_void()) {
						op_stack.push((uint64_t)result);
					}
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


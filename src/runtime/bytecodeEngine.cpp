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
#include "system_directory.hpp"
#include "classloader.hpp"
#include "native/native.hpp"
#include "native/java_lang_String.hpp"
#include <memory>
#include <sstream>
#include <functional>
#include <boost/any.hpp>
#include <map>
#include <utility>
#include "utils/synchronize_wcout.hpp"
#include "runtime/thread.hpp"

using std::wstringstream;
using std::list;
using std::function;
using std::shared_ptr;
using std::map;
using std::pair;
using std::make_pair;

/**
 * 小技能：
 * 把 log 中不含 "<bytecode>" 的日志行全部正则删除的方法QAQ
 * 见 http://www.cnblogs.com/wangqiguo/archive/2012/05/08/2486548.html
 * 用 ^(?!.*<bytecode>).*\n 这个零度替换(?!)，替换为 "" 就可以......
 * 调 bug 要分析几万行的日志。。。。。。想死的心都有了QAQQAQQAQ
 */

/**
 * bug 手记：今天又一次触发 EXC_BAD_ACCESS (code=2, address=0x7fff5f3ff910) 错误。
 * 错误点在于：前一步的函数 xxx(*this, ...) 在参数传递之后，*this 就变了......
 * 找了半天错误，竟然惊人的发现是触发了无限循环？？为什么会这样......
 * 太可恨了这狗日错误！！说爆栈不就好了吗？！还 EXC_BAD_ACCESS??? code=2?????? 卧槽...... /翻白眼
 * 你直接说是 Stack Overflow 不就行了吗！！
 *
 * 总结下经验......以后看到 Segmentation Fault 还是使用 echo $? 查看返回值吧......
 * 刚才的 StackOverflow 是 139.
 * assert(false) 是 134.
 * 用 perror 134 / perror 139 查看信息......虽然没有卵用
 */

/**
 * 新建一个 list, 不小心调用了 list.popfront()，报错：
 * malloc: *** error for object 0x70000f6da2d0: pointer being freed was not allocated
 * *** set a breakpoint in malloc_error_break to debug
 *
 * 确实在 pop_front() 的时候要调用析构函数。毕竟 STL 的值是 [复制] 进去的。
 */

/*===----------- StackFrame --------------===*/
StackFrame::StackFrame(shared_ptr<Method> method, uint8_t *return_pc, StackFrame *prev, const list<Oop *> & args, bool is_native) : method(method), return_pc(return_pc), prev(prev) {	// va_args is: Method's argument. 所有的变长参数的类型全是有类型的 Oop。因此，在**执行 code**的时候就会有类型检查～
	if (is_native) {
		return;
	}
	localVariableTable.resize(method->get_code()->max_locals);
	int i = 0;	// 注意：这里的 vector 采取一开始就分配好大小的方式。因为后续过程中不可能有 push_back 存在。因为字节码都是按照 max_local 直接对 localVariableTable[i] 进行调用的。
	for (Oop * value : args) {
		// 在这里，会把 localVariableTable 按照规范，long 和 double 会自动占据两位。
		localVariableTable.at(i++) = value;	// 检查越界。
		if (value != nullptr && value->get_klass() != nullptr && value->get_klass()->get_name() == L"java/lang/String") {		// 特例：如果是 String，就打出来～
#ifdef DEBUG
			sync_wcout{} << "the "<< i-1 << " argument is java/lang/String: [" << java_lang_string::stringOop_to_wstring((InstanceOop *)value) << "]" << std::endl;
#endif
		}
		if (value != nullptr && value->get_klass() != nullptr && value->get_klass()->get_name() == L"java/lang/Class") {			// 特例：如果是 Class，就打出来～
			wstring type = ((MirrorOop *)value)->get_mirrored_who() == nullptr ? ((MirrorOop *)value)->get_extra() : ((MirrorOop *)value)->get_mirrored_who()->get_name();
#ifdef DEBUG
			sync_wcout{} << "the "<< i-1 << " argument is java/lang/Class: [" << type << "]" << std::endl;
#endif
		}
		if (value != nullptr && value->get_ooptype() == OopType::_BasicTypeOop
				&& ((((BasicTypeOop *)value)->get_type() == Type::LONG) || (((BasicTypeOop *)value)->get_type() == Type::DOUBLE))) {
			localVariableTable.at(i++) = nullptr;
		}
	}
}

void StackFrame::clear_all() {					// used with `is_valid()`. if invalid, clear all to reuse this frame.
	this->valid_frame = true;
	stack<Oop *>().swap(op_stack);				// empty. [Effective STL]
	vector<Oop *>().swap(localVariableTable);
	method = nullptr;
	return_pc = nullptr;
	// prev not change.
}

/*===------------ BytecodeEngine ---------------===*/

vector<wstring> BytecodeEngine::parse_arg_list(const wstring & descriptor)
{
	vector<wstring> arg_list;
	for (int i = 1; i < descriptor.size(); i ++) {		// ignore the first L'('.
		// define
		function<void(wchar_t)> recursive_arg = [&arg_list, &descriptor, &recursive_arg, &i](wchar_t cur) {
			switch (descriptor[i]) {
				case L'Z':{
					arg_list.push_back(L"Z");
					break;
				}
				case L'B':{
					arg_list.push_back(L"B");
					break;
				}
				case L'C':{
					arg_list.push_back(L"C");
					break;
				}
				case L'S':{
					arg_list.push_back(L"S");
					break;
				}
				case L'I':{
					arg_list.push_back(L"I");
					break;
				}
				case L'F':{
					arg_list.push_back(L"F");
					break;
				}
				case L'J':{
					arg_list.push_back(L"J");
					break;
				}
				case L'D':{
					arg_list.push_back(L"D");
					break;
				}
				case L'L':{
					int origin = i;
					while(descriptor[i] != L';') {
						i ++;
					}
//					arg_list.push_back(descriptor.substr(origin + 1, i - origin - 1));	// 不包括 L 和 ;.	// deprecated.
					arg_list.push_back(descriptor.substr(origin, i - origin + 1));	// 包括 L 和 ;.	// 变成 klass 的时候再删除。
					break;
				}
				case L'[':{
					while(descriptor[i] == L'[') {
						i ++;
					}
					recursive_arg(descriptor[i]);	// will push_back a wrong answer
					wstring temp = L"[" + arg_list.back();
					arg_list.pop_back();				// will pop_back it.
					arg_list.push_back(temp);
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
#ifdef DEBUG
	sync_wcout{} << "===------------------  arg list (" << descriptor << ")  ----------------===" << std::endl;
	for (int i = 0; i < arg_list.size(); i ++) {
		sync_wcout{} << i << ". " << arg_list[i] << std::endl;
	}
	sync_wcout{} << "===---------------------------------------------------------===" << std::endl;
#endif
	return arg_list;
}

wstring BytecodeEngine::get_real_value(Oop *oop)
{
	if (oop == nullptr) return L"null";
	wstringstream ss;
	if (oop->get_klass() == nullptr) {		// basic type oop.
		switch (((BasicTypeOop *)oop)->get_type()) {
			case Type::BYTE:
			case Type::BOOLEAN:
			case Type::SHORT:
			case Type::INT:
				ss << ((IntOop *)oop)->value;
				break;
			case Type::CHAR:
				ss << (wchar_t)((IntOop *)oop)->value;
				break;
			case Type::FLOAT:
				ss << ((FloatOop *)oop)->value;
				break;
			case Type::LONG:
				ss << ((LongOop *)oop)->value;
				break;
			case Type::DOUBLE:
				ss << ((DoubleOop *)oop)->value;
				break;
			default:{
				std::cerr << "can't get here!" << std::endl;
				assert(false);
			}
		}
	} else ss << oop;						// if it is a reference, return itself.

	return ss.str();
}

Oop* copy_BasicOop_value(Oop *oop)		// aux
{
	if (oop->get_ooptype() == OopType::_BasicTypeOop) {
		BasicTypeOop *basic = (BasicTypeOop *)oop;
		switch(basic->get_type()) {
			case Type::BOOLEAN:
			case Type::BYTE:
			case Type::SHORT:
			case Type::CHAR:
			case Type::INT:
				return new IntOop(((IntOop *)basic)->value);
			case Type::DOUBLE:
				return new DoubleOop(((DoubleOop *)basic)->value);
			case Type::FLOAT:
				return new FloatOop(((DoubleOop *)basic)->value);
			case Type::LONG:
				return new LongOop(((DoubleOop *)basic)->value);
			default:
				assert(false);
		}
	} else {
		assert(false);
	}
}

Oop *if_BasicType_then_copy_else_return_only(Oop *oop)
{
	if (oop != nullptr && oop->get_ooptype() == OopType::_BasicTypeOop) {
		return copy_BasicOop_value(oop);		// 我设置如果是 BasicType 类型，那就复制一份 value。
	} else {
		return oop;							// 如果是 Reference 类型，那就直接 指针复制。
	}
}

bool BytecodeEngine::check_instanceof(shared_ptr<Klass> ref_klass, shared_ptr<Klass> klass)
{
	bool result;
	if (ref_klass->get_type() == ClassType::InstanceClass) {
		if (ref_klass->is_interface()) {		// a. ref_klass is an interface
			if (klass->is_interface()) {		// a1. klass is an interface, too
				if (ref_klass == klass || std::static_pointer_cast<InstanceKlass>(ref_klass)->check_interfaces(std::static_pointer_cast<InstanceKlass>(klass))) {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " and klass " << klass->get_name() << " are both interfaces. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			} else {							// a2. klass is a normal class
				if (klass->get_name() == L"java/lang/Object") {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " is interface but klass " << klass->get_name() << " is normal class. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			}
		} else {								// b. ref_klass is a normal class
			if (klass->is_interface()) {		// b1. klass is an interface
				if (std::static_pointer_cast<InstanceKlass>(ref_klass)->check_interfaces(std::static_pointer_cast<InstanceKlass>(klass))) {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " is normal class but klass " << klass->get_name() << " is an interface. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			} else {							// b2. klass is a normal class, too
				if (ref_klass == klass || std::static_pointer_cast<InstanceKlass>(ref_klass)->check_parent(std::static_pointer_cast<InstanceKlass>(klass))) {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " and klass " << klass->get_name() << " are both normal classes. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			}
		}
	} else if (ref_klass->get_type() == ClassType::TypeArrayClass || ref_klass->get_type() == ClassType::ObjArrayClass) {	// c. ref_klass is an array
		if (klass->get_type() == ClassType::InstanceClass) {
			if (!klass->is_interface()) {		// c1. klass is a normal class
				if (klass->get_name() == L"java/lang/Object") {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " is an array but klass " << klass->get_name() << " is a normal classes. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			} else {								// c2. klass is an interface		// Please see JLS $4.10.3	// array default implements: 1. java/lang/Cloneable  2. java/io/Serializable
				if (klass->get_name() == L"java/lang/Cloneable" || klass->get_name() == L"java/io/Serializable") {
					result = true;
				} else {
					result = false;
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " is an array but klass " << klass->get_name() << " is an interface. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			}
		} else if (klass->get_type() == ClassType::TypeArrayClass) {		// c3. klass is an TypeArrayKlass
			// 1. 编译器保证 ref 和 klass 的 dimension 必然相同。
			int ref_dimension = (std::static_pointer_cast<ArrayKlass>(ref_klass))->get_dimension();
			int klass_dimension = (std::static_pointer_cast<ArrayKlass>(klass))->get_dimension();
			assert (ref_dimension == klass_dimension);
			// 2. judge
			if (ref_klass->get_type() == ClassType::TypeArrayClass && std::static_pointer_cast<TypeArrayKlass>(klass)->get_basic_type() == std::static_pointer_cast<TypeArrayKlass>(ref_klass)->get_basic_type()) {
				result = true;
			} else {
				result = false;
			}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " and klass " << klass->get_name() << " are both arrays. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			return result;
		} else if (klass->get_type() == ClassType::ObjArrayClass) {		// c4. klass is an ObjArrayKlass
			// 1. 编译器保证 ref 和 klass 的 dimension 必然相同。
			int ref_dimension = (std::static_pointer_cast<ArrayKlass>(ref_klass))->get_dimension();
			int klass_dimension = (std::static_pointer_cast<ArrayKlass>(klass))->get_dimension();
			assert (ref_dimension == klass_dimension);
			// 2. judge
			if (ref_klass->get_type() == ClassType::ObjArrayClass) {
				auto ref_klass_inner_type = std::static_pointer_cast<ObjArrayKlass>(ref_klass)->get_element_klass();
				auto klass_inner_type = std::static_pointer_cast<ObjArrayKlass>(klass)->get_element_klass();
				if (ref_klass_inner_type == klass_inner_type || std::static_pointer_cast<InstanceKlass>(ref_klass_inner_type)->check_parent(std::static_pointer_cast<InstanceKlass>(klass_inner_type))) {
					result = true;
				} else {
					result = false;
				}
			} else {
				result = false;
			}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref_klass: " << ref_klass->get_name() << " and klass " << klass->get_name() << " are both arrays. [`instanceof` is " << std::boolalpha << result << "]" << std::endl;
#endif
			return result;
		} else {
			assert(false);
		}
	} else {
		assert(false);
	}
	return result;
}

void BytecodeEngine::initial_clinit(shared_ptr<InstanceKlass> new_klass, vm_thread & thread)
{
	if (new_klass->get_state() == Klass::KlassState::NotInitialized) {
#ifdef DEBUG
		sync_wcout{} << "initializing <class>: [" << new_klass->get_name() << "]" << std::endl;
#endif
		new_klass->set_state(Klass::KlassState::Initializing);		// important.
		// recursively initialize its parent first !!!!! So java.lang.Object must be the first !!!
		if (new_klass->get_parent() != nullptr)	// prevent this_klass is the java.lang.Object.
			BytecodeEngine::initial_clinit(std::static_pointer_cast<InstanceKlass>(new_klass->get_parent()), thread);
		// if static field has ConstantValue_attribute (final field), then initialize it.
		new_klass->initialize_final_static_field();
		// then initialize this_klass, call <clinit>.
#ifdef DEBUG
		sync_wcout{} << "(DEBUG) " << new_klass->get_name() << "::<clinit>" << std::endl;
#endif
		shared_ptr<Method> clinit = new_klass->get_this_class_method(L"<clinit>:()V");		// **IMPORTANT** only search in this_class for `<clinit>` !!!
		if (clinit != nullptr) {		// TODO: 这里 clinit 不知道会如何执行。
			thread.add_frame_and_execute(clinit, {});		// no return value
		} else {
#ifdef DEBUG
			sync_wcout{} << "(DEBUG) no <clinit>." << std::endl;
#endif
		}
		new_klass->set_state(Klass::KlassState::Initialized);
	}
}

// TODO: 注意！每个指令 pc[1] 如果是 byte，可能指向常量池第几位什么的，本来应该是一个无符号数，但是我全用 int 承接的！所以有潜在的风险！！！
// TODO: 注意！！以下，所有代码，不应该出现 ByteOop、BooleanOop、ShortOop ！！ 取而代之的应当是 IntOop ！！
Oop * BytecodeEngine::execute(vm_thread & thread, StackFrame & cur_frame, int thread_no) {		// 卧槽......vector 由于扩容，会导致内部的引用全部失效...... 改成 list 吧......却是忽略了这点。

	assert(&cur_frame == &thread.vm_stack.back());
	shared_ptr<Method> method = cur_frame.method;
	uint32_t code_length = method->get_code()->code_length;
	stack<Oop *> & op_stack = cur_frame.op_stack;
	vector<Oop *> & localVariableTable = cur_frame.localVariableTable;
	uint8_t *code_begin = method->get_code()->code;
	shared_ptr<InstanceKlass> klass = method->get_klass();
	rt_constant_pool & rt_pool = *klass->get_rtpool();

	uint8_t *backup_pc = thread.pc;
	uint8_t * & pc = thread.pc;
	pc = code_begin;

#ifdef DEBUG
	sync_wcout{} << "[Now, it's StackFrame #" << thread.vm_stack.size() - 1 << "]." << std::endl;
#endif

	while (pc < code_begin + code_length) {
#ifdef DEBUG
		sync_wcout{} << L"(DEBUG) [thread " << thread_no << "] <bytecode> $" << std::dec <<  (pc - code_begin) << " of "<< klass->get_name() << "::" << method->get_name() << ":" << method->get_descriptor() << " --> " << utf8_to_wstring(bccode_map[*pc].first) << std::endl;
#endif
		int occupied = bccode_map[*pc].second + 1;
		switch(*pc) {
			case 0x00:{		// nop
				// do nothing.
				break;
			}
			case 0x01:{		// aconst_null
				op_stack.push(nullptr);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push null on stack." << std::endl;
#endif
				break;
			}
			case 0x02:{		// iconst_m1
				op_stack.push(new IntOop(-1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int -1 on stack." << std::endl;
#endif
				break;
			}
			case 0x03:{		// iconst_0
				op_stack.push(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 0 on stack." << std::endl;
#endif
				break;
			}
			case 0x04:{		// iconst_1
				op_stack.push(new IntOop(1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 1 on stack." << std::endl;
#endif
				break;
			}
			case 0x05:{		// iconst_2
				op_stack.push(new IntOop(2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 2 on stack." << std::endl;
#endif
				break;
			}
			case 0x06:{		// iconst_3
				op_stack.push(new IntOop(3));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 3 on stack." << std::endl;
#endif
				break;
			}
			case 0x07:{		// iconst_4
				op_stack.push(new IntOop(4));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 4 on stack." << std::endl;
#endif
				break;
			}
			case 0x08:{		// iconst_5
				op_stack.push(new IntOop(5));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int 5 on stack." << std::endl;
#endif
				break;
			}
			case 0x09:{		// lconst_0
				op_stack.push(new LongOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push long 0 on stack." << std::endl;
#endif
				break;
			}
			case 0x0a:{		// lconst_1
				op_stack.push(new LongOop(1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push long 1 on stack." << std::endl;
#endif
				break;
			}
			case 0x0b:{		// fconst_0
				op_stack.push(new FloatOop((float)0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push float 0.0f on stack." << std::endl;
#endif
				break;
			}
			case 0x0c:{		// fconst_1
				op_stack.push(new FloatOop((float)1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push float 1.0f on stack." << std::endl;
#endif
				break;
			}
			case 0x0d:{		// fconst_2
				op_stack.push(new FloatOop((float)2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push float 2.0f on stack." << std::endl;
#endif
				break;
			}
			case 0x0e:{		// dconst_1
				op_stack.push(new DoubleOop((double)1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push double 1.0ld on stack." << std::endl;
#endif
				break;
			}
			case 0x0f:{		// dconst_2
				op_stack.push(new DoubleOop((double)2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push double 2.0ld on stack." << std::endl;
#endif
				break;
			}
			case 0x10: {		// bipush
				op_stack.push(new IntOop(pc[1]));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push byte " << ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x11:{		// sipush
				short val = ((pc[1] << 8) | pc[2]);
				op_stack.push(new IntOop(val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push short " << ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x12:		// ldc
			case 0x13:{		// ldc_w
				int rtpool_index;
				if (*pc == 0x12) {
					rtpool_index = pc[1];
				} else {
					rtpool_index = ((pc[1] << 8) | pc[2]);
				}
				if (rt_pool[rtpool_index-1].first == CONSTANT_Integer) {
					int value = boost::any_cast<int>(rt_pool[rtpool_index-1].second);
					op_stack.push(new IntOop(value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push int: "<< value << " on stack." << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_Float) {
					float value = boost::any_cast<float>(rt_pool[rtpool_index-1].second);
					op_stack.push(new FloatOop(value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push float: "<< ((FloatOop *)op_stack.top())->value << "f on stack." << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_String) {
					InstanceOop *stringoop = (InstanceOop *)boost::any_cast<Oop *>(rt_pool[rtpool_index-1].second);
					op_stack.push(stringoop);
#ifdef DEBUG
	// for string:
	sync_wcout{} << java_lang_string::print_stringOop(stringoop) << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_Class) {
					auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);
					assert(klass->get_mirror() != nullptr);
					op_stack.push(klass->get_mirror());		// push into [Oop*] type.
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push class: "<< klass->get_name() << "'s mirror "<< "on stack." << std::endl;
#endif
				} else {
					// TODO: Constant_MethodHandle and Constant_MethodType
					std::cerr << "doesn't support Constant_MethodHandle and Constant_MethodType now..." << std::endl;
					assert(false);
				}
				break;
			}
			case 0x14:{		// ldc2_w
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				if (rt_pool[rtpool_index-1].first == CONSTANT_Double) {
					double value = boost::any_cast<double>(rt_pool[rtpool_index-1].second);
					op_stack.push(new DoubleOop(value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push double: "<< value << "ld on stack." << std::endl;
#endif
				} else if (rt_pool[rtpool_index-1].first == CONSTANT_Long) {
					long value = boost::any_cast<long>(rt_pool[rtpool_index-1].second);
					op_stack.push(new LongOop(value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push long: "<< value << "l on stack." << std::endl;
#endif
				} else {
					assert(false);
				}
				break;
			}
			case 0x15:{		// iload
				int index = pc[1];
				assert(localVariableTable.size() > index && index > 3);	// 如果是 3 以下，那么会用 iload_0~3.
				assert(localVariableTable[index]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[index])->get_type() == Type::INT);
				op_stack.push(new IntOop(((IntOop *)localVariableTable[index])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[" << index << "] int: "<< ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x16:{		// lload
				int index = pc[1];
				assert(localVariableTable.size() > index && index > 3);	// 如果是 3 以下，那么会用 lload_0~3.
				assert(localVariableTable[index]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[index])->get_type() == Type::LONG);
				op_stack.push(new LongOop(((LongOop *)localVariableTable[index])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[" << index << "] long: "<< ((LongOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x17:{		// fload		// bus error bug 的发生地！
//				assert(false);			// 然后我在前边加了一句 assert(false)，来确定程序真的没走到这。没想到加了这一句之后，程序竟然通了！这说明加了 assert(false) 之后，clang++ 的某项优化应该失效了，所以才正常了。到时候要对比一下汇编码了。
				int index = pc[1];
				assert(localVariableTable.size() > index && index > 3);	// 如果是 3 以下，那么会用 fload_0~3.
				assert(localVariableTable[index]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[index])->get_type() == Type::FLOAT);
				op_stack.push(new FloatOop(((FloatOop *)localVariableTable[index])->value));
#ifdef DEBUG		// 去掉这一段，bus error bug 匪夷所思地消失了... 然而程序根本没走到这啊？？？？！！到底是什么情况？？？？看来应该是编译器优化的原因吗......
	sync_wcout{} << "(DEBUG) push localVariableTable[" << index << "] float: "<< ((FloatOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x18:{		// dload
				int index = pc[1];
				assert(localVariableTable.size() > index && index > 3);	// 如果是 3 以下，那么会用 dload_0~3.
				assert(localVariableTable[index]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[index])->get_type() == Type::DOUBLE);
				op_stack.push(new DoubleOop(((DoubleOop *)localVariableTable[index])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[" << index << "] double: "<< ((DoubleOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x19:{		// aload
				int index = pc[1];
				assert(localVariableTable.size() > index && index > 3);	// 如果是 3 以下，那么会用 aload_0~3.
				if (localVariableTable[index] != nullptr)	assert(localVariableTable[index]->get_ooptype() != OopType::_BasicTypeOop);		// 因为也有可能是数组。所以只要保证不是基本类型就行。
				op_stack.push(localVariableTable[index]);
#ifdef DEBUG
	if (localVariableTable[index] != nullptr)
		sync_wcout{} << "(DEBUG) push localVariableTable[" << index << "] ref: "<< (localVariableTable[index])->get_klass()->get_name() << "'s Oop: address: " << std::hex << localVariableTable[index] << " on stack." << std::endl;
	else
		sync_wcout{} << "(DEBUG) push <null> ref from localVariableTable[" << index << "], to stack." << std::endl;
#endif
				break;
			}
			case 0x1a:{		// iload_0
				assert(localVariableTable[0]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[0])->get_type() == Type::INT);
				op_stack.push(new IntOop(((IntOop *)localVariableTable[0])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[0] int: "<< ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x1b:{		// iload_1
				assert(localVariableTable[1]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[1])->get_type() == Type::INT);
				op_stack.push(new IntOop(((IntOop *)localVariableTable[1])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[1] int: "<< ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x1c:{		// iload_2
				assert(localVariableTable[2]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[2])->get_type() == Type::INT);
				op_stack.push(new IntOop(((IntOop *)localVariableTable[2])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[2] int: "<< ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x1d:{		// iload_3
				assert(localVariableTable[3]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[3])->get_type() == Type::INT);
				op_stack.push(new IntOop(((IntOop *)localVariableTable[3])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[3] int: "<< ((IntOop *)op_stack.top())->value << " on stack." << std::endl;
#endif
				break;
			}
			case 0x1e:{		// lload_0
				assert(localVariableTable[0]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[0])->get_type() == Type::LONG);
				op_stack.push(new LongOop(((LongOop *)localVariableTable[0])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[0] long: "<< ((LongOop *)op_stack.top())->value << "l on stack." << std::endl;
#endif
				break;
			}
			case 0x1f:{		// lload_1
				assert(localVariableTable[1]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[1])->get_type() == Type::LONG);
				op_stack.push(new LongOop(((LongOop *)localVariableTable[1])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[1] long: "<< ((LongOop *)op_stack.top())->value << "l on stack." << std::endl;
#endif
				break;
			}
			case 0x20:{		// lload_2
				assert(localVariableTable[2]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[2])->get_type() == Type::LONG);
				op_stack.push(new LongOop(((LongOop *)localVariableTable[2])->value));
#ifdef DEBUG						// 是的，还有这里也是！去掉之后就没事。但是会触发另一个非常诡异的 segmentation fault. linux 平台没有此现象......
	sync_wcout{} << "(DEBUG) push localVariableTable[2] long: "<< ((LongOop *)op_stack.top())->value << "l on stack." << std::endl;
#endif
				break;
			}
			case 0x21:{		// lload_3
				assert(localVariableTable[3]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[3])->get_type() == Type::LONG);
				op_stack.push(new LongOop(((LongOop *)localVariableTable[3])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[3] long: "<< ((LongOop *)op_stack.top())->value << "l on stack." << std::endl;
#endif
				break;
			}
			case 0x22:{		// fload_0
				assert(localVariableTable[0]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[0])->get_type() == Type::FLOAT);
				op_stack.push(new FloatOop(((FloatOop *)localVariableTable[0])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[0] float: "<< ((FloatOop *)op_stack.top())->value << "f on stack." << std::endl;
#endif
				break;
			}
			case 0x23:{		// fload_1
				assert(localVariableTable[1]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[1])->get_type() == Type::FLOAT);
				op_stack.push(new FloatOop(((FloatOop *)localVariableTable[1])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[1] float: "<< ((FloatOop *)op_stack.top())->value << "f on stack." << std::endl;
#endif
				break;
			}
			case 0x24:{		// fload_2
				assert(localVariableTable[2]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[2])->get_type() == Type::FLOAT);
				op_stack.push(new FloatOop(((FloatOop *)localVariableTable[2])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[2] float: "<< ((FloatOop *)op_stack.top())->value << "f on stack." << std::endl;
#endif
				break;
			}
			case 0x25:{		// fload_3
				assert(localVariableTable[3]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[3])->get_type() == Type::FLOAT);
				op_stack.push(new FloatOop(((FloatOop *)localVariableTable[3])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[3] float: "<< ((FloatOop *)op_stack.top())->value << "f on stack." << std::endl;
#endif
				break;
			}
			case 0x26:{		// dload_0
				assert(localVariableTable[0]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[0])->get_type() == Type::DOUBLE);
				op_stack.push(new DoubleOop(((DoubleOop *)localVariableTable[0])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[0] double: "<< ((DoubleOop *)op_stack.top())->value << "ld on stack." << std::endl;
#endif
				break;
			}
			case 0x27:{		// dload_1
				assert(localVariableTable[1]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[1])->get_type() == Type::DOUBLE);
				op_stack.push(new DoubleOop(((DoubleOop *)localVariableTable[1])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[1] double: "<< ((DoubleOop *)op_stack.top())->value << "ld on stack." << std::endl;
#endif
				break;
			}
			case 0x28:{		// dload_2
				assert(localVariableTable[2]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[2])->get_type() == Type::DOUBLE);
				op_stack.push(new DoubleOop(((DoubleOop *)localVariableTable[2])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[2] double: "<< ((DoubleOop *)op_stack.top())->value << "ld on stack." << std::endl;
#endif
				break;
			}
			case 0x29:{		// dload_3
				assert(localVariableTable[3]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[3])->get_type() == Type::DOUBLE);
				op_stack.push(new DoubleOop(((DoubleOop *)localVariableTable[3])->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) push localVariableTable[3] double: "<< ((DoubleOop *)op_stack.top())->value << "ld on stack." << std::endl;
#endif
				break;
			}



			case 0x2a:{		// aload_0
				if (localVariableTable[0] != nullptr)	assert(localVariableTable[0]->get_ooptype() != OopType::_BasicTypeOop);
				op_stack.push(localVariableTable[0]);
#ifdef DEBUG
	if (localVariableTable[0] != nullptr)
		sync_wcout{} << "(DEBUG) push localVariableTable[0] ref: "<< (localVariableTable[0])->get_klass()->get_name() << "'s Oop: address: " << std::hex << localVariableTable[0] << " on stack." << std::endl;
	else
		sync_wcout{} << "(DEBUG) push <null> ref from localVariableTable[0], to stack." << std::endl;
#endif
				break;
			}
			case 0x2b:{		// aload_1
				if (localVariableTable[1] != nullptr)	assert(localVariableTable[1]->get_ooptype() != OopType::_BasicTypeOop);
				op_stack.push(localVariableTable[1]);
#ifdef DEBUG
	if (localVariableTable[1] != nullptr)
		sync_wcout{} << "(DEBUG) push localVariableTable[1] ref: "<< (localVariableTable[1])->get_klass()->get_name() << "'s Oop: address: " << std::hex << localVariableTable[1] << " on stack." << std::endl;
	else
		sync_wcout{} << "(DEBUG) push <null> ref from localVariableTable[1], to stack." << std::endl;
#endif
				break;
			}
			case 0x2c:{		// aload_2
				if (localVariableTable[2] != nullptr)	assert(localVariableTable[2]->get_ooptype() != OopType::_BasicTypeOop);
				op_stack.push(localVariableTable[2]);
#ifdef DEBUG
	if (localVariableTable[2] != nullptr)
		sync_wcout{} << "(DEBUG) push localVariableTable[2] ref: "<< (localVariableTable[2])->get_klass()->get_name() << "'s Oop: address: " << std::hex << localVariableTable[2] << " on stack." << std::endl;
	else
		sync_wcout{} << "(DEBUG) push <null> ref from localVariableTable[2], to stack." << std::endl;
#endif
				break;
			}
			case 0x2d:{		// aload_3
				if (localVariableTable[3] != nullptr)	assert(localVariableTable[3]->get_ooptype() != OopType::_BasicTypeOop);
				op_stack.push(localVariableTable[3]);
#ifdef DEBUG
	if (localVariableTable[3] != nullptr)
		sync_wcout{} << "(DEBUG) push localVariableTable[3] ref: "<< (localVariableTable[3])->get_klass()->get_name() << "'s Oop: address: " << std::hex << localVariableTable[3] << " on stack." << std::endl;
	else
		sync_wcout{} << "(DEBUG) push <null> ref from localVariableTable[3], to stack." << std::endl;
#endif
				break;
			}
			case 0x2e:{		// iaload
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[I");		// assert int[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				op_stack.push(new IntOop(((IntOop *)((*charsequence)[index]))->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get int[" << index << "] which is the int: [" << ((IntOop *)op_stack.top())->value << "]" << std::endl;
#endif
				break;
			}
			case 0x2f:{		// laload
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[J");		// assert long[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				op_stack.push(new LongOop(((IntOop *)((*charsequence)[index]))->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get long[" << index << "] which is the long: [" << ((LongOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}



			case 0x32:{		// aaload
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_ObjArrayOop);		// assert char[] array
				ObjArrayOop * objarray = (ObjArrayOop *)op_stack.top();	op_stack.pop();
				assert(objarray->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				op_stack.push((*objarray)[index]);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get ObjArray[" << index << "] which type is <class>" << std::static_pointer_cast<ObjArrayKlass>(objarray->get_klass())->get_element_klass()->get_name()
			   << ", and the element address is "<< op_stack.top() << "." << std::endl;
#endif
				break;
			}
			case 0x33:{		// baload
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && (op_stack.top()->get_klass()->get_name() == L"[B" || op_stack.top()->get_klass()->get_name() == L"[Z"));		// assert byte[]/boolean[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				op_stack.push(new IntOop(((IntOop *)((*charsequence)[index]))->value));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get byte/boolean[" << index << "] which is the byte/boolean: [" << ((IntOop *)op_stack.top())->value << "]" << std::endl;
#endif
				break;
			}
			case 0x34:{		// caload
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[C");		// assert char[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
#ifdef DEBUG
				sync_wcout{} << charsequence->get_length() << " " << index << std::endl;		// delete
#endif
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				op_stack.push(new IntOop(((IntOop *)((*charsequence)[index]))->value));			// TODO: ...... 我取消了 CharOop，因此造成了没法扩展......一定要回来重看...
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get char[" << index << "] which is the wchar_t: [" << (wchar_t)((IntOop *)op_stack.top())->value << "]" << std::endl;
#endif
				break;
			}

			case 0x36:{		// istore
				int index = pc[1];
				assert(index > 3);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				IntOop *ref = (IntOop *)op_stack.top();
				localVariableTable[index] = new IntOop(ref->value);	op_stack.pop();
#ifdef DEBUG
		sync_wcout{} << "(DEBUG) pop int [" << ref->value << "] from stack, to localVariableTable[" << index << "]." << std::endl;
#endif
				break;
			}
			case 0x37:{		// lstore
				int index = pc[1];
				assert(index > 3);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				LongOop *ref = (LongOop *)op_stack.top();
				localVariableTable[index] = new LongOop(ref->value);	op_stack.pop();
#ifdef DEBUG
		sync_wcout{} << "(DEBUG) pop long [" << ref->value << "] from stack, to localVariableTable[" << index << "]." << std::endl;
#endif
				break;
			}
			case 0x38:{		// fstore
				int index = pc[1];
				assert(index > 3);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				FloatOop *ref = (FloatOop *)op_stack.top();
				localVariableTable[index] = new FloatOop(ref->value);	op_stack.pop();
#ifdef DEBUG
		sync_wcout{} << "(DEBUG) pop float [" << ref->value << "] from stack, to localVariableTable[" << index << "]." << std::endl;
#endif
				break;
			}
			case 0x39:{		// dstore
				int index = pc[1];
				assert(index > 3);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				DoubleOop *ref = (DoubleOop *)op_stack.top();
				localVariableTable[index] = new DoubleOop(ref->value);	op_stack.pop();
#ifdef DEBUG
		sync_wcout{} << "(DEBUG) pop double [" << ref->value << "] from stack, to localVariableTable[" << index << "]." << std::endl;
#endif
				break;
			}


			case 0x3a:{		// astore
				int index = pc[1];
				assert(index > 3);
				if (op_stack.top() != nullptr)	assert(op_stack.top()->get_ooptype() != OopType::_BasicTypeOop);
				Oop *ref = op_stack.top();
				localVariableTable[index] = ref;	op_stack.pop();
#ifdef DEBUG
	if (ref != nullptr)	// ref == null
		sync_wcout{} << "(DEBUG) pop ref from stack, "<< ref->get_klass()->get_name() << "'s Oop: address: " << std::hex << ref << " to localVariableTable[" << index << "]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) pop <null> ref from stack, to localVariableTable[" << index << "]." << std::endl;
#endif
				break;
			}
			case 0x3b:{		// istore_0
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				localVariableTable[0] = new IntOop(((IntOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top int: "<< ((IntOop *)localVariableTable[0])->value << " to localVariableTable[0] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x3c:{		// istore_1
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				localVariableTable[1] = new IntOop(((IntOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top int: "<< ((IntOop *)localVariableTable[1])->value << " to localVariableTable[1] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x3d:{		// istore_2
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				localVariableTable[2] = new IntOop(((IntOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top int: "<< ((IntOop *)localVariableTable[2])->value << " to localVariableTable[2] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x3e:{		// istore_3
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				localVariableTable[3] = new IntOop(((IntOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top int: "<< ((IntOop *)localVariableTable[3])->value << " to localVariableTable[3] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x3f:{		// lstore_0
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				localVariableTable[0] = new LongOop(((LongOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top long: "<< ((LongOop *)localVariableTable[0])->value << " to localVariableTable[0] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x40:{		// lstore_1
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				localVariableTable[1] = new LongOop(((LongOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top long: "<< ((LongOop *)localVariableTable[1])->value << " to localVariableTable[1] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x41:{		// lstore_2
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				localVariableTable[2] = new LongOop(((LongOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top long: "<< ((LongOop *)localVariableTable[2])->value << " to localVariableTable[2] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x42:{		// lstore_3
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				localVariableTable[3] = new LongOop(((LongOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top long: "<< ((LongOop *)localVariableTable[3])->value << " to localVariableTable[3] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x43:{		// fstore_0
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				localVariableTable[0] = new FloatOop(((FloatOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top float: "<< ((FloatOop *)localVariableTable[0])->value << " to localVariableTable[0] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x44:{		// fstore_1
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				localVariableTable[1] = new FloatOop(((FloatOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top float: "<< ((FloatOop *)localVariableTable[1])->value << " to localVariableTable[1] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x45:{		// fstore_2
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				localVariableTable[2] = new FloatOop(((FloatOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top float: "<< ((FloatOop *)localVariableTable[2])->value << " to localVariableTable[2] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x46:{		// fstore_3
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				localVariableTable[3] = new FloatOop(((FloatOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top float: "<< ((FloatOop *)localVariableTable[3])->value << " to localVariableTable[3] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x47:{		// dstore_0
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				localVariableTable[0] = new DoubleOop(((DoubleOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top double: "<< ((DoubleOop *)localVariableTable[0])->value << " to localVariableTable[0] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x48:{		// dstore_1
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				localVariableTable[1] = new DoubleOop(((DoubleOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top double: "<< ((DoubleOop *)localVariableTable[1])->value << " to localVariableTable[1] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x49:{		// dstore_2
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				localVariableTable[2] = new DoubleOop(((DoubleOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top double: "<< ((DoubleOop *)localVariableTable[2])->value << " to localVariableTable[2] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x4a:{		// dstore_3
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				localVariableTable[3] = new DoubleOop(((DoubleOop *)op_stack.top())->value);	op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) pop stack top double: "<< ((DoubleOop *)localVariableTable[3])->value << " to localVariableTable[3] and rewrite." << std::endl;
#endif
				break;
			}
			case 0x4b:{		// astore_0
				if (op_stack.top() != nullptr)	assert(op_stack.top()->get_ooptype() != OopType::_BasicTypeOop);
				Oop *ref = op_stack.top();
				localVariableTable[0] = op_stack.top();	op_stack.pop();
#ifdef DEBUG
	if (ref != nullptr)	// ref == null
		sync_wcout{} << "(DEBUG) pop ref from stack, "<< ref->get_klass()->get_name() << "'s Oop: address: " << std::hex << ref << " to localVariableTable[0]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) pop <null> ref from stack, to localVariableTable[0]." << std::endl;
#endif
				break;
			}
			case 0x4c:{		// astore_1
				if (op_stack.top() != nullptr)	assert(op_stack.top()->get_ooptype() != OopType::_BasicTypeOop);
				Oop *ref = op_stack.top();
				localVariableTable[1] = op_stack.top();	op_stack.pop();
#ifdef DEBUG
	if (ref != nullptr)	// ref == null
		sync_wcout{} << "(DEBUG) pop ref from stack, "<< ref->get_klass()->get_name() << "'s Oop: address: " << std::hex << ref << " to localVariableTable[1]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) pop <null> ref from stack, to localVariableTable[1]." << std::endl;
#endif
				break;
			}
			case 0x4d:{		// astore_2
				// bug report: 这里出现过一次 EXC_BAD_ACCESS (code=EXC_I386_GPFLT)。原因是因为访问了一块不属于自己的内存。但是其实真正原因是：op_stack.size() == 0。特此记录。
				if (op_stack.top() != nullptr)	assert(op_stack.top()->get_ooptype() != OopType::_BasicTypeOop);
				Oop *ref = op_stack.top();
				localVariableTable[2] = op_stack.top();	op_stack.pop();
#ifdef DEBUG
	if (ref != nullptr)	// ref == null
		sync_wcout{} << "(DEBUG) pop ref from stack, "<< ref->get_klass()->get_name() << "'s Oop: address: " << std::hex << ref << " to localVariableTable[2]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) pop <null> ref from stack, to localVariableTable[2]." << std::endl;
#endif
				break;
			}
			case 0x4e:{		// astore_3
				if (op_stack.top() != nullptr)	assert(op_stack.top()->get_ooptype() != OopType::_BasicTypeOop);
				Oop *ref = op_stack.top();
				localVariableTable[3] = op_stack.top();	op_stack.pop();
#ifdef DEBUG
	if (ref != nullptr)	// ref == null
		sync_wcout{} << "(DEBUG) pop ref from stack, "<< ref->get_klass()->get_name() << "'s Oop: address: " << std::hex << ref << " to localVariableTable[3]." << std::endl;
	else
		sync_wcout{} << "(DEBUG) pop <null> ref from stack, to localVariableTable[3]." << std::endl;
#endif
				break;
			}
			case 0x4f:{		// iastore
				IntOop *value = (IntOop *)op_stack.top();	op_stack.pop();
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_ObjArrayOop || op_stack.top()->get_ooptype() == OopType::_TypeArrayOop);
				ArrayOop *arr = (ArrayOop *)op_stack.top();	op_stack.pop();
				assert(index < arr->get_length());
				(*arr)[index] = new IntOop(value->value);	// 不能指向同一个对象了...。理论上有问题...。
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) put an int value: [" << value->value << "] into the IntArray[" << index << "]." << std::endl;
#endif
				break;
			}


			case 0x50:{		// lastore
				LongOop *value = (LongOop *)op_stack.top();	op_stack.pop();
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[J");		// assert char[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				(*charsequence)[index] = new LongOop(value->value);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get long ['" << value->value << "'] from the stack to long[]'s position of [" << index << "]" << std::endl;
#endif
				break;
			}

			case 0x53:{		// aastore
				Oop *value = op_stack.top();	op_stack.pop();
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				Oop *array_ref = op_stack.top();	op_stack.pop();
				assert(array_ref != nullptr && array_ref->get_ooptype() == OopType::_ObjArrayOop);
//				if (value != nullptr) {
//					sync_wcout{} << value->get_ooptype() << ((ObjArrayOop *)array_ref)->get_dimension() << std::endl;
//					assert(value->get_ooptype() == OopType::_InstanceOop);		// 不做检查了。因为也会有把 一个[[[Ljava.lang.String; 放到 [Ljava.lang.Object;的 [1] 中去的。
//				}
				InstanceOop *real_value = (InstanceOop *)value;
				ObjArrayOop *real_array = (ObjArrayOop *)array_ref;
				assert(real_array->get_length() > index);
				// overwrite
				(*real_array)[index] = value;
#ifdef DEBUG
	if (real_value == nullptr) {
		sync_wcout{} << "(DEBUG) put <null> into the index [" << index << "] of the ObjArray of type: ["
				   << std::static_pointer_cast<ObjArrayKlass>(real_array->get_klass())->get_element_klass() << "]" << std::endl;
	} else {
		sync_wcout{} << "(DEBUG) put element of type: [" << real_value->get_klass()->get_name() << "], address :[" << real_value << "] into the index [" << index
				   << "] of the ObjArray of type: [" << std::static_pointer_cast<ObjArrayKlass>(real_array->get_klass())->get_element_klass() << "]" << std::endl;
	}
#endif
				break;
			}
			case 0x54:{		// bastore
				IntOop *byte = (IntOop *)op_stack.top();	op_stack.pop();
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[B");		// assert byte[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				(*charsequence)[index] = new IntOop((int)((char)(byte->value)));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get byte/boolean ['" << (int)((char)byte->value) << "'] from the stack to byte/boolean[]'s position of [" << index << "]" << std::endl;
#endif
				break;
			}
			case 0x55:{		// castore
				IntOop *ch = (IntOop *)op_stack.top();	op_stack.pop();
				int index = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (op_stack.top() == nullptr) {
					// TODO: should throw NullpointerException
					assert(false);
				}
				assert(op_stack.top()->get_ooptype() == OopType::_TypeArrayOop && op_stack.top()->get_klass()->get_name() == L"[C");		// assert char[] array
				TypeArrayOop * charsequence = (TypeArrayOop *)op_stack.top();	op_stack.pop();
				assert(charsequence->get_length() > index && index >= 0);	// TODO: should throw ArrayIndexOutofBoundException
				(*charsequence)[index] = new IntOop((unsigned short)ch->value);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get wchar_t ['" << ch->value << "'] from the stack to char[]'s position of [" << index << "]" << std::endl;
#endif
				break;
			}


			case 0x57:{		// pop
				op_stack.pop();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) only pop from stack." << std::endl;
#endif
				break;
			}

			case 0x59:{		// dup
				op_stack.push(if_BasicType_then_copy_else_return_only(op_stack.top()));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) only dup from stack." << std::endl;
#endif
				break;
			}
			case 0x5a:{		// dup_x1
				Oop *val1 = op_stack.top();	op_stack.pop();
				Oop *val2 = op_stack.top();	op_stack.pop();
				op_stack.push(if_BasicType_then_copy_else_return_only(val1));
				op_stack.push(val2);
				op_stack.push(val1);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) only dup_x1 from stack." << std::endl;
#endif
				break;
			}
			case 0x5b:{		// dup_x2
				Oop *val1 = op_stack.top();	op_stack.pop();
				Oop *val2 = op_stack.top();	op_stack.pop();
				if (val2->get_ooptype() == OopType::_BasicTypeOop && 		// 分类二
						(((BasicTypeOop *)val2)->get_type() == Type::LONG || ((BasicTypeOop *)val2)->get_type() == Type::DOUBLE)) {
					// case 2
					op_stack.push(if_BasicType_then_copy_else_return_only(val1));
					op_stack.push(val2);
					op_stack.push(val1);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) dup_x2[2] from stack." << std::endl;
#endif
				} else {
					// case 1
					Oop *val3 = op_stack.top();	op_stack.pop();
					op_stack.push(if_BasicType_then_copy_else_return_only(val1));
					op_stack.push(val3);
					op_stack.push(val2);
					op_stack.push(val1);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) dup_x2[1] from stack." << std::endl;
#endif
				}
				break;
			}
			case 0x5c:{		// dup2
				Oop *val1 = op_stack.top();	op_stack.pop();
				if (val1->get_ooptype() == OopType::_BasicTypeOop && 		// 分类二
						(((BasicTypeOop *)val1)->get_type() == Type::LONG || ((BasicTypeOop *)val1)->get_type() == Type::DOUBLE)) {
					op_stack.push(if_BasicType_then_copy_else_return_only(val1));
					op_stack.push(val1);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) dup_2[1] from stack." << std::endl;
#endif
				} else {
					Oop *val2 = op_stack.top();	op_stack.pop();
					op_stack.push(if_BasicType_then_copy_else_return_only(val2));
					op_stack.push(if_BasicType_then_copy_else_return_only(val1));
					op_stack.push(val2);
					op_stack.push(val1);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) dup_2[2] from stack." << std::endl;
#endif
				}
				break;
			}


			case 0x5e:{		// dup2_x2
				Oop *val1 = op_stack.top();	op_stack.pop();
				Oop *val2 = op_stack.top();	op_stack.pop();
				if (val1->get_ooptype() == OopType::_BasicTypeOop &&
						(((BasicTypeOop *)val1)->get_type() == Type::LONG || ((BasicTypeOop *)val1)->get_type() == Type::DOUBLE)) {	// 1. val1 是 分类2
					if (val2->get_ooptype() == OopType::_BasicTypeOop &&
						(((BasicTypeOop *)val2)->get_type() == Type::LONG || ((BasicTypeOop *)val2)->get_type() == Type::DOUBLE)) {	// 1-a. val2 也是 分类2
						// case 4
						op_stack.push(if_BasicType_then_copy_else_return_only(val1));		// TODO: !!! 是不是不安全...按照 primitive type 默认复制的原则，是不是应该从弹栈的那一刻开始，原先的 value 就应该失效，就应该复制一份！！！
						op_stack.push(val2);
						op_stack.push(val1);
					} else {		// 1-b. val2 是 分类1，当然 val3 也是。
						// case 2
						Oop *val3 = op_stack.top();	op_stack.pop();
						op_stack.push(if_BasicType_then_copy_else_return_only(val1));
						op_stack.push(val3);
						op_stack.push(val2);
						op_stack.push(val1);
					}
				} else {		// 2. val1 是 分类1，余下的情况中，val2 必然也是 分类1
					Oop *val3 = op_stack.top();	op_stack.pop();
					if (val3->get_ooptype() == OopType::_BasicTypeOop &&
							(((BasicTypeOop *)val3)->get_type() == Type::LONG || ((BasicTypeOop *)val3)->get_type() == Type::DOUBLE)) {	// 2-a. val3 是 分类2
						// case 3
						op_stack.push(if_BasicType_then_copy_else_return_only(val2));
						op_stack.push(if_BasicType_then_copy_else_return_only(val1));
						op_stack.push(val3);
						op_stack.push(val2);
						op_stack.push(val1);
					} else {		// 2-b. val3 是 分类1
						// case 1
						Oop *val4 = op_stack.top();	op_stack.pop();
						op_stack.push(if_BasicType_then_copy_else_return_only(val2));
						op_stack.push(if_BasicType_then_copy_else_return_only(val1));
						op_stack.push(val4);
						op_stack.push(val3);
						op_stack.push(val2);
						op_stack.push(val1);
					}
				}

				break;
			}


			case 0x60:{		// iadd
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val2 + val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) add int value from stack: "<< val2 << " + " << val1 << " and put " << (val2+val1) << " on stack." << std::endl;
#endif
				break;
			}
			case 0x61:{		// ladd
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val2 + val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) add long value from stack: "<< val2 << " + " << val1 << " and put " << (val2+val1) << " on stack." << std::endl;
#endif
				break;
			}


			case 0x63:{		// dadd
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val2 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val1 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();

#ifdef DEBUG
	auto print_double = [](double val) {
		if (val == DOUBLE_NAN)						sync_wcout{} << "NAN";
		else if (val == DOUBLE_INFINITY)				sync_wcout{} << "DOUBLE_INFINITY";
		else if (val == DOUBLE_NEGATIVE_INFINITY)		sync_wcout{} << "DOUBLE_NEGATIVE_INFINITY";
		else sync_wcout{} << val << "ld";
	};
	sync_wcout{} << "(DEBUG) dadd of val2: [";
	print_double(val2);
	sync_wcout{} << "] and val1: [";
	print_double(val1);
	sync_wcout{} << "], result is: [";
#endif
				if (val2 == DOUBLE_NAN || val1 == DOUBLE_NAN) {
					op_stack.push(new DoubleOop(NAN));
				} else if ((val2 == DOUBLE_INFINITY && val1 == DOUBLE_NEGATIVE_INFINITY) || (val1 == DOUBLE_INFINITY && val2 == DOUBLE_NEGATIVE_INFINITY)) {
					op_stack.push(new DoubleOop(NAN));
				} else if (val2 == DOUBLE_INFINITY && val1 == DOUBLE_INFINITY) {
					op_stack.push(new DoubleOop(DOUBLE_INFINITY));
				} else if (val2 == DOUBLE_NEGATIVE_INFINITY && val1 == DOUBLE_NEGATIVE_INFINITY) {
					op_stack.push(new DoubleOop(DOUBLE_NEGATIVE_INFINITY));
				} else if (val2 == DOUBLE_INFINITY && (val1 != DOUBLE_INFINITY && val1 != DOUBLE_NAN && val1 != DOUBLE_NEGATIVE_INFINITY)) {
					op_stack.push(new DoubleOop(DOUBLE_INFINITY));
				} else if (val2 == DOUBLE_NEGATIVE_INFINITY && (val1 != DOUBLE_INFINITY && val1 != DOUBLE_NAN && val1 != DOUBLE_NEGATIVE_INFINITY)) {
					op_stack.push(new DoubleOop(DOUBLE_NEGATIVE_INFINITY));
				} else if (val1 == DOUBLE_INFINITY && (val2 != DOUBLE_INFINITY && val2 != DOUBLE_NAN && val2 != DOUBLE_NEGATIVE_INFINITY)) {
					op_stack.push(new DoubleOop(DOUBLE_INFINITY));
				} else if (val1 == DOUBLE_NEGATIVE_INFINITY && (val2 != DOUBLE_INFINITY && val2 != DOUBLE_NAN && val2 != DOUBLE_NEGATIVE_INFINITY)) {
					op_stack.push(new DoubleOop(DOUBLE_NEGATIVE_INFINITY));
				} else {	// TODO: ???? 相同符号的零值？？？ 不同符号的零值？？？？
					double result = val2 + val1;
					op_stack.push(new DoubleOop(result));

//					if (val2 > 0 && val1 > 0 && result < 0) {				// judge overflow:		// TODO: wrong algorithm......除了用汇编直接读取，如何判断是否溢出 ????
//						op_stack.push(new DoubleOop(DOUBLE_INFINITY));
//					} else if (val2 < 0 && val1 < 0 && result > 0) {		// judge underflow:
//						op_stack.push(new DoubleOop(DOUBLE_NEGATIVE_INFINITY));
//					} else {												// else: no flow.
//						op_stack.push(new DoubleOop(result));
//					}
				}

#ifdef DEBUG
	print_double(((DoubleOop *)op_stack.top())->value);
	sync_wcout{} << "]." << std::endl;
#endif

				break;
			}
			case 0x64:{		// isub
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val1 - val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) sub int value from stack: "<< val1 << " - " << val2 << "(on top) and put " << (val1-val2) << " on stack." << std::endl;
#endif
				break;
			}
			case 0x65:{		// lsub
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val1 - val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) sub long value from stack: "<< val1 << " - " << val2 << "(on top) and put " << (val1-val2) << " on stack." << std::endl;
#endif
				break;
			}



			case 0x67:{		// dsub		// TODO: 正负 0.0 到底怎么回事？？OWO
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val2 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val1 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new DoubleOop(val1 - val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) sub double value from stack: "<< val1 << " - " << val2 << "(on top) and put " << (val1-val2) << " on stack." << std::endl;
#endif
				break;
			}
			case 0x68:{		// imul
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val1 * val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) mul int value from stack: "<< val1 << " * " << val2 << "(on top) and put " << (val1 * val2) << " on stack." << std::endl;
#endif
				break;
			}
			case 0x69:{		// lmul
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val1 * val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) mul long value from stack: "<< val1 << " * " << val2 << "(on top) and put " << (val1 * val2) << " on stack." << std::endl;
#endif
				break;
			}



			case 0x6a:{		// fmul
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val2 = ((FloatOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val1 = ((FloatOop*)op_stack.top())->value; op_stack.pop();
#ifdef DEBUG
	auto print_float = [](float val) {
		if (val == FLOAT_NAN)	sync_wcout{} << "NAN";
		else if (val == FLOAT_INFINITY)	sync_wcout{} << "FLOAT_INFINITY";
		else if (val == FLOAT_NEGATIVE_INFINITY)	sync_wcout{} << "FLOAT_NEGATIVE_INFINITY";
		else sync_wcout{} << val << "f";
	};
	sync_wcout{} << "(DEBUG) fmul of val2: [";
	print_float(val2);
	sync_wcout{} << "] and val1: [";
	print_float(val1);
	sync_wcout{} << "], result is: [";
#endif
				if (val2 == FLOAT_NAN || val1 == FLOAT_NAN) {		// NAN * (any other)
					op_stack.push(new FloatOop(FLOAT_NAN));		// [x] 不兼容.....[x] 注意 cmath 中的 INFINITY, -INFINITY 以及 NAN 全是针对 fp 浮点数来说的！！
				} else if (((val2 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY) && val1 == 0.0f) || ((val1 == FLOAT_INFINITY || val1 == FLOAT_NEGATIVE_INFINITY) && val2 == 0.0f)) {
					op_stack.push(new FloatOop(FLOAT_NAN));			// INFINITY * 0
				} else if ((val2 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY) && (val1 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY)) {
					if ((val1 == FLOAT_INFINITY && val2 == FLOAT_INFINITY) || (val1 == FLOAT_NEGATIVE_INFINITY && val2 == FLOAT_NEGATIVE_INFINITY)) {
						op_stack.push(new FloatOop(FLOAT_INFINITY));
					} else {											// INFINITY * INFINITY
						op_stack.push(new FloatOop(FLOAT_NEGATIVE_INFINITY));
					}
				} else {
					float result = val2 * val1;
					op_stack.push(new FloatOop(result));
				}
#ifdef DEBUG
	print_float(((FloatOop *)op_stack.top())	->value);
	sync_wcout{} << "]." << std::endl;
#endif
				break;
			}
			case 0x6b:{		// dmul
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val2 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val1 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();

#ifdef DEBUG
	auto print_double = [](double val) {
		if (val == DOUBLE_NAN)	sync_wcout{} << "NAN";
		else if (val == DOUBLE_INFINITY)	sync_wcout{} << "DOUBLE_INFINITY";
		else if (val == DOUBLE_NEGATIVE_INFINITY)	sync_wcout{} << "DOUBLE_NEGATIVE_INFINITY";
		else sync_wcout{} << val;
	};
	sync_wcout{} << "(DEBUG) dmul of val2: [";
	print_double(val2);
	sync_wcout{} << "] and val1: [";
	print_double(val1);
	sync_wcout{} << "], result is: [";
#endif
				if (val2 == DOUBLE_NAN || val1 == DOUBLE_NAN) {		// NAN * (any other)
					op_stack.push(new DoubleOop(DOUBLE_NAN));		// [x] 不兼容.....[x] 注意 cmath 中的 INFINITY, -INFINITY 以及 NAN 全是针对 fp 浮点数来说的！！
				} else if (((val2 == DOUBLE_INFINITY || val2 == DOUBLE_NEGATIVE_INFINITY) && val1 == 0.0) || ((val1 == DOUBLE_INFINITY || val1 == DOUBLE_NEGATIVE_INFINITY) && val2 == 0.0)) {
					op_stack.push(new DoubleOop(DOUBLE_NAN));			// INFINITY * 0
				} else if ((val2 == DOUBLE_INFINITY || val2 == DOUBLE_NEGATIVE_INFINITY) && (val1 == DOUBLE_INFINITY || val2 == DOUBLE_NEGATIVE_INFINITY)) {
					if ((val1 == DOUBLE_INFINITY && val2 == DOUBLE_INFINITY) || (val1 == DOUBLE_NEGATIVE_INFINITY && val2 == DOUBLE_NEGATIVE_INFINITY)) {
						op_stack.push(new DoubleOop(DOUBLE_INFINITY));
					} else {											// INFINITY * INFINITY
						op_stack.push(new DoubleOop(DOUBLE_NEGATIVE_INFINITY));
					}
				} else {
					double result = val2 * val1;
					op_stack.push(new DoubleOop(result));
				}
#ifdef DEBUG
	print_double(((DoubleOop *)op_stack.top())->value);
	sync_wcout{} << "]." << std::endl;
#endif
				break;
			}
			case 0x6c:{		// idiv
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();		// 不 delete。由 GC 一块来。
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val1 / val2));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) sub int value from stack: "<< val1 << " / " << val2 << "(on top) and put " << (val1 / val2) << " on stack." << std::endl;
#endif
				break;
			}



			case 0x6e:{		// fdiv
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val2 = ((FloatOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val1 = ((FloatOop*)op_stack.top())->value; op_stack.pop();

#ifdef DEBUG
	auto print_float = [](float val) {
		if (val == FLOAT_NAN)	sync_wcout{} << "FLOAT_NAN";
		else if (val == FLOAT_INFINITY)	sync_wcout{} << "FLOAT_INFINITY";
		else if (val == FLOAT_NEGATIVE_INFINITY)	sync_wcout{} << "FLOAT_NEGATIVE_INFINITY";
		else sync_wcout{} << val << "f";
	};
	sync_wcout{} << "(DEBUG) fdiv of val2: [";
	print_float(val2);
	sync_wcout{} << "] and val1: [";
	print_float(val1);
	sync_wcout{} << "], result is: [";
#endif
				if (val2 == FLOAT_NAN || val1 == FLOAT_NAN) {		// NAN / (any other)
					op_stack.push(new FloatOop(FLOAT_NAN));
				} else if ((val2 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY) && (val1 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY)) {
					op_stack.push(new FloatOop(FLOAT_NAN));			// INFINITY / INFINITY
				} else if ((val2 == FLOAT_INFINITY || val2 == FLOAT_NEGATIVE_INFINITY) && (val1 != FLOAT_NAN && val1 != FLOAT_INFINITY && val1 != FLOAT_NEGATIVE_INFINITY)) {
					if ((val2 < 0 && val1 < 0) || (val2 > 0 && val1 > 0)) {
						op_stack.push(new FloatOop(FLOAT_INFINITY));			// INFINITY / non-INFINITY
					} else {
						op_stack.push(new FloatOop(FLOAT_NEGATIVE_INFINITY));			// INFINITY / non-INFINITY
					}
				} else if ((val2 != FLOAT_NAN && val2 != FLOAT_INFINITY && val2 != FLOAT_NEGATIVE_INFINITY) && (val1 == FLOAT_INFINITY || val1 == FLOAT_NEGATIVE_INFINITY)) {
					op_stack.push(new FloatOop(0.0f));					// non-INFINITY / INFINITY		// TODO: 这里不太明白。规范上说 零值 也有符号，虽然不是没听过，还是难以理解...
				} else if (val1 == 0.0 && val2 == 0.0) {
					op_stack.push(new FloatOop(FLOAT_NAN));
				} else if (val2 == 0.0 && (val1 != FLOAT_NAN && val1 != FLOAT_INFINITY && val1 != FLOAT_NEGATIVE_INFINITY && val1 != 0.0)) {
					if (val1 > 0)
						op_stack.push(new FloatOop(FLOAT_INFINITY));
					else
						op_stack.push(new FloatOop(FLOAT_NEGATIVE_INFINITY));
				} else if ((val2 != FLOAT_NAN && val2 != FLOAT_INFINITY && val2 != FLOAT_NEGATIVE_INFINITY && val2 != 0.0) && val1 == 0.0) {
					if (val2 > 0)
						op_stack.push(new FloatOop(FLOAT_INFINITY));
					else
						op_stack.push(new FloatOop(FLOAT_NEGATIVE_INFINITY));
				} else {
					op_stack.push(new FloatOop(val2 / val1));
				}
#ifdef DEBUG
	print_float(((FloatOop *)op_stack.top())	->value);
	sync_wcout{} << "]." << std::endl;
#endif
				break;
			}


			case 0x70:{		// irem
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();

				assert(val2 != 0);
				assert((val1 / val2) * val2 + (val1 % val2) == val1);
				assert(val1 % val2 == (val1 - (val1 / val2) * val2));

				op_stack.push(new IntOop(val1 % val2));

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " % " << val2 << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x71:{		// lrem
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();

				assert(val2 != 0);
				assert((val1 / val2) * val2 + (val1 % val2) == val1);
				assert(val1 % val2 == (val1 - (val1 / val2) * val2));

				op_stack.push(new LongOop(val1 % val2));

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " % " << val2 << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}



			case 0x74:{		// ineg
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(-val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [ -" << val << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x75:{		// lneg
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(-val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [ -" << val << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}



			case 0x78:{		// ishl
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				int s = (val2 & 0x1F);
				op_stack.push(new IntOop(val1 << s));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " << " << s << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x79:{		// lshl
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				int s = (val2 & 0x3F);
				op_stack.push(new LongOop(val1 << s));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " << " << s << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}


			case 0x7a:{		// ishr
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				int s = (val2 & 0x1F);
				op_stack.push(new IntOop(val1 >> s));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " >> " << s << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x7b:{		// lshr
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				int s = (val2 & 0x3F);
				op_stack.push(new LongOop(val1 >> s));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " >> " << s << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x7c:{		// iushr
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();

				int s = (val2 & 0x1F);
				if (val1 >= 0) {
					op_stack.push(new IntOop(val1 >> s));
				} else {
					op_stack.push(new IntOop((val1 >> s)+(2 << ~s)));
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " >> " << s << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x7d:{		// lushr
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();

				int s = (val2 & 0x3F);
				if (val1 >= 0) {
					op_stack.push(new LongOop(val1 >> s));
				} else {
					op_stack.push(new LongOop((val1 >> s)+(2l << ~s)));
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val1 << " >> " << s << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x7e:{		// iand
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val2 & val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " & " << val1 << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x7f:{		// land
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val2 & val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " & " << val1 << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x80:{		// ior
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val2 | val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " | " << val1 << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x81:{		// lor
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val2 | val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " | " << val1 << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x82:{		// ixor
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val2 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val1 = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop(val2 ^ val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " ^ " << val1 << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x83:{		// lxor
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop(val2 ^ val1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) do [" << val2 << " ^ " << val1 << "], result is " << ((LongOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x84:{		// iinc
				int index = pc[1];
				int _const = (int8_t)pc[2];			// TODO: 这里！！千万小心！！严格按照规范来！！pc[2] 是有符号的！！如果值是 -1，然而我设置的 pc 是 uint_8 ！！直接转成 int 会变成 255！！！必须先变成 int8_t 才是 -1！！！
				assert(localVariableTable[index]->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)localVariableTable[index])->get_type() == Type::INT);
				((IntOop *)localVariableTable[index])->value += _const;		// TODO: 注意！！这里直接修改，而不是自增了！！上次说要指向同一个的那个...
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) localVariableTable[" << index << "] += " << _const << ", now value is [" << ((IntOop *)localVariableTable[index])->value << "]." << std::endl;
#endif
				break;
			}
			case 0x85:{		// i2l
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new LongOop((long)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to long: [" << ((LongOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x86:{		// i2f
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new FloatOop((float)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to float: [" << ((FloatOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x87:{		// i2d
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new DoubleOop((double)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to double: [" << ((DoubleOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x88:{		// l2i
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop((int)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert long: [" << val << "] to int: [" << ((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x89:{		// l2f
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val = ((LongOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new FloatOop((float)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert long: [" << val << "] to float: [" << ((FloatOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}


			case 0x8b:{		// f2i		// TODO: 和 fmul, getField, setField 一样，没有做 Spec $2.8.3 FP Strict!!
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val = ((FloatOop*)op_stack.top())->value; op_stack.pop();

				if (val == FLOAT_NAN) {
					op_stack.push(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert float: [FLOAT_NAN] to int: [0]." << std::endl;
#endif
				} else if (val == FLOAT_INFINITY) {
					op_stack.push(new IntOop(INT_MAX));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert float: [FLOAT_INFINITY] to int: [INT_MAX]." << std::endl;
#endif
				} else if (val == FLOAT_NEGATIVE_INFINITY) {
					op_stack.push(new IntOop(INT_MIN));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert float: [FLOAT_NEGATIVE_INFINITY] to int: [INT_MIN]." << std::endl;
#endif
				} else {
					op_stack.push(new IntOop((int)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert float: [" << val << "f] to int: [" << ((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				}

				break;
			}

			case 0x8d:{		// f2d
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val = ((FloatOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new DoubleOop((double)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert float: [" << val << "f] to double: [" << ((DoubleOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x8e:{		// d2i
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val = ((DoubleOop*)op_stack.top())->value; op_stack.pop();

				if (val == DOUBLE_NAN) {
					op_stack.push(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_NAN] to int: [0]." << std::endl;
#endif
				} else if (val == DOUBLE_INFINITY) {
					op_stack.push(new IntOop(INT_MAX));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_INFINITY] to int: [INT_MAX]." << std::endl;
#endif
				} else if (val == DOUBLE_NEGATIVE_INFINITY) {
					op_stack.push(new IntOop(INT_MIN));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_NEGATIVE_INFINITY] to int: [INT_MIN]." << std::endl;
#endif
				} else {
					op_stack.push(new IntOop((int)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [" << val << "ld] to int: [" << ((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				}
				break;
			}
			case 0x8f:{		// d2l
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val = ((DoubleOop*)op_stack.top())->value; op_stack.pop();

				if (val == DOUBLE_NAN) {
					op_stack.push(new LongOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_NAN] to long: [0]." << std::endl;
#endif
				} else if (val == DOUBLE_INFINITY) {
					op_stack.push(new LongOop(LONG_MAX));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_INFINITY] to long: [LONG_MAX]." << std::endl;
#endif
				} else if (val == DOUBLE_NEGATIVE_INFINITY) {
					op_stack.push(new LongOop(LONG_MIN));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [DOUBLE_NEGATIVE_INFINITY] to long: [LONG_MIN]." << std::endl;
#endif
				} else {
					op_stack.push(new LongOop((long)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert double: [" << val << "ld] to long: [" << ((LongOop *)op_stack.top())->value << "]." << std::endl;
#endif
				}
				break;
			}

			case 0x91:{		// i2b
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop((char)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to byte: [" << std::dec << ((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x92:{		// i2c
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop((unsigned short)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to char: [" << (wchar_t)((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x93:{		// i2s
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int val = ((IntOop*)op_stack.top())->value; op_stack.pop();
				op_stack.push(new IntOop((short)val));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) convert int: [" << val << "] to short: [" << ((IntOop *)op_stack.top())->value << "]." << std::endl;
#endif
				break;
			}
			case 0x94:{		// lcmp
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val2 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
				long val1 = ((LongOop*)op_stack.top())->value; op_stack.pop();
				if (val2 > val1) {
					op_stack.push(new IntOop(-1));
				} else if (val2 < val1) {
					op_stack.push(new IntOop(1));
				} else {
					op_stack.push(new IntOop(0));
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) compare longs: [" << val1 << " and " << val2 << "], result is " << ((IntOop *)op_stack.top())->value << "." << std::endl;
#endif
				break;
			}
			case 0x95:		// fcmp_l
			case 0x96:{		// fcmp_g
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val2 = ((FloatOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
				float val1 = ((FloatOop*)op_stack.top())->value; op_stack.pop();

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ";
#endif
				if (val1 == FLOAT_NAN || val2 == FLOAT_NAN) {
					if (*pc == 0x95) {
						op_stack.push(new IntOop(-1));
					} else {
						op_stack.push(new IntOop(1));
					}
#ifdef DEBUG
	sync_wcout{} << "meet FLOAT_NAN. then ";
#endif
				} else if (val1 < val2) {
					op_stack.push(new IntOop(-1));
				} else if (val1 > val2) {
					op_stack.push(new IntOop(1));
				} else {
					op_stack.push(new IntOop(0));
				}
#ifdef DEBUG
	sync_wcout{} << "push [" << ((IntOop *)op_stack.top())->value << "] onto the stack." << std::endl;
#endif
				break;
			}
			case 0x97:		// dcmpl
			case 0x98:{		// dcmpg
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val2 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
				double val1 = ((DoubleOop*)op_stack.top())->value; op_stack.pop();

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ";
#endif
				if (val1 == DOUBLE_NAN || val2 == DOUBLE_NAN) {
					if (*pc == 0x97) {
						op_stack.push(new IntOop(-1));
					} else {
						op_stack.push(new IntOop(1));
					}
#ifdef DEBUG
	sync_wcout{} << "meet DOUBLE_NAN. then ";
#endif
				} else if (val1 < val2) {
					op_stack.push(new IntOop(-1));
				} else if (val1 > val2) {
					op_stack.push(new IntOop(1));
				} else {
					op_stack.push(new IntOop(0));
				}
#ifdef DEBUG
	sync_wcout{} << "push [" << ((IntOop *)op_stack.top())->value << "] onto the stack." << std::endl;
#endif
				break;
			}
			case 0x99:		// ifeq
			case 0x9a:		// ifne
			case 0x9b:		// iflt
			case 0x9c:		// ifge
			case 0x9d:		// ifgt
			case 0x9e:{		// ifle
				short branch_pc = ((pc[1] << 8) | pc[2]);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int int_value = ((IntOop*)op_stack.top())->value;	op_stack.pop();
				bool judge;
				if (*pc == 0x99) {
					judge = (int_value == 0);
				} else if (*pc == 0x9a) {
					judge = (int_value != 0);
				} else if (*pc == 0x9b) {
					judge = (int_value < 0);
				} else if (*pc == 0x9c) {
//					judge = (int_value <= 0);	// delete		// 这里是最诡异的 bug 的发现地点......太厉害了。用 clang++ 和 g++ 编译，clang++ 跑到一半崩溃；g++ 一直跑都没事。简直神了。然后调试 bug 一直调试不出来，看哪好像都是对的。mdzz。
					judge = (int_value >= 0);	// real
				} else if (*pc == 0x9d) {
					judge = (int_value > 0);
				} else {
//					judge = (int_value >= 0);	// delete
					judge = (int_value <= 0);	// real
				}

				if (judge) {	// if true, jump to the branch_pc.
					pc += branch_pc;
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) int value is " << int_value << ", so will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {		// if false, go next.
					// do nothing
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) int value is " << int_value << ", so will go next." << std::endl;
#endif
				}
				break;
			}
			case 0x9f:		// if_icmpeq
			case 0xa0:		// if_icmpne
			case 0xa1:		// if_icmplt
			case 0xa2:		// if_icmpge
			case 0xa3:		// if_icmpgt
			case 0xa4:{		// if_icmple
				short branch_pc = ((pc[1] << 8) | pc[2]);
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int value2 = ((IntOop*)op_stack.top())->value;	op_stack.pop();
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int value1 = ((IntOop*)op_stack.top())->value;	op_stack.pop();
				bool judge;
				if (*pc == 0x9f) {
					judge = (value1 == value2);
				} else if (*pc == 0xa0) {
					judge = (value1 != value2);
				} else if (*pc == 0xa1) {
					judge = (value1 < value2);
				} else if (*pc == 0xa2) {
//					judge = (value1 <= value2);			// delete
					judge = (value1 >= value2);			// real
				} else if (*pc == 0xa3) {					// mdzz 这里全都没改然后出了 EXC_BAD_ACCESS Code = 2 错误......
					judge = (value1 > value2);
				} else {
//					judge = (value1 >= value2);			// delete
					judge = (value1 <= value2);			// real
				}

				if (judge) {	// if true, jump to the branch_pc.
					pc += branch_pc;
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) int value compare from stack is " << value2 << " and " << value1 << ", so will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {		// if false, go next.
					// do nothing
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) int value compare from stack is " << value2 << " and " << value1 << ", so will go next." << std::endl;
#endif
				}
				break;
			}
			case 0xa5:		// if_acmpeq		// 应该是仅仅比较 ref 所引用的地址。
			case 0xa6:{		// if_acmpne
				short branch_pc = ((pc[1] << 8) | pc[2]);
				Oop *value2 = op_stack.top();	op_stack.pop();
				Oop *value1 = op_stack.top();	op_stack.pop();
				bool judge;
				if (*pc == 0xa5) {
					judge = (value1 == value2);
				} else {
					judge = (value1 != value2);
				}

				if (judge) {	// if true, jump to the branch_pc.
					pc += branch_pc;
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref value compare from stack is " << value2 << " and " << value1 << ", so will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {		// if false, go next.
					// do nothing
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref value compare from stack is " << value2 << " and " << value1 << ", so will go next." << std::endl;
#endif
				}
				break;
			}
			case 0xa7:{		// goto
				short branch_pc = ((pc[1] << 8) | pc[2]);		// [重大bug] 用 short！！！不能用 int！！！ 因为这里如果想要变负，pc[1] 和 pc[2] 全是 uint8_t，想要变负，就必须要
															// 强制让它越界！！而我一开始用 int，本意是害怕让它越界，结果反而越不了界了...... 要全改！！
				pc += branch_pc;
				pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) will [goto]: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				break;
			}


			case 0xaa:{		// tableswitch
				int bc_num = pc - code_begin;
				uint8_t *code = code_begin;
				int origin_bc_num = bc_num;
				if (bc_num % 4 != 0) {
					bc_num += (4 - bc_num % 4);
				} else {
					bc_num += 4;
				}
				int ptr = bc_num;

				// calculate basic values
				int defaultbyte = ((code[ptr] << 24) | (code[ptr+1] << 16) | (code[ptr+2] << 8) | (code[ptr+3]));
				int lowbyte = ((code[ptr+4] << 24) | (code[ptr+5] << 16) | (code[ptr+6] << 8) | (code[ptr+7]));
				int highbyte = ((code[ptr+8] << 24) | (code[ptr+9] << 16) | (code[ptr+10] << 8) | (code[ptr+11]));
//						std::cout << defaultbyte << " " << lowbyte << " " << highbyte << std::endl;	// delete
				int num = highbyte - lowbyte + 1;
				ptr += 12;
				// create jump_table
				vector<int> jump_tbl;
				for (int pos = 0; pos < num; pos ++) {
					int jump_pos = ((code[ptr] << 24) | (code[ptr+1] << 16) | (code[ptr+2] << 8) | (code[ptr+3])) + origin_bc_num;
					ptr += 4;
					jump_tbl.push_back(jump_pos);
				}
				jump_tbl.push_back(defaultbyte + origin_bc_num);		// 额外多 push 一个 default 跳转址
				// jump begin~
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int key = ((IntOop *)op_stack.top())->value;
				if (key > jump_tbl.size() - 1 + lowbyte || key < lowbyte) {
					// goto default
					pc = (code_begin + jump_tbl.back());
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) it is [tableswitch] switch(" << key << ") {...}, didn't match... so will jump to [default]: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {
					pc = (code_begin + jump_tbl[key - lowbyte]);
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) it is [tableswitch] switch(" << key << ") {...}, so will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				}
				break;
			}
			case 0xab:{		// lookupswitch
				int bc_num = pc - code_begin;
				uint8_t *code = code_begin;
				int origin_bc_num = bc_num;
				if (bc_num % 4 != 0) {
					bc_num += (4 - bc_num % 4);
				} else {
					bc_num += 4;
				}
				int ptr = bc_num;
				// calculate basic values
				int defaultbyte = ((code[ptr] << 24) | (code[ptr+1] << 16) | (code[ptr+2] << 8) | (code[ptr+3]));
				int npairs = ((code[ptr+4] << 24) | (code[ptr+5] << 16) | (code[ptr+6] << 8) | (code[ptr+7]));
				ptr += 8;
				// create jump_table
				map<int, int> jump_tbl;
				for (int pos = 0; pos < npairs; pos ++) {
					int match_value = ((code[ptr] << 24) | (code[ptr+1] << 16) | (code[ptr+2] << 8) | (code[ptr+3]));
					int jump_pos = ((code[ptr+4] << 24) | (code[ptr+5] << 16) | (code[ptr+6] << 8) | (code[ptr+7])) + origin_bc_num;
					ptr += 8;
					jump_tbl.insert(make_pair(match_value, jump_pos));
				}
				jump_tbl.insert(make_pair(INT_MAX, defaultbyte + origin_bc_num));		// 额外多 insert 一个 default 跳转址
				// jump begin~
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
				int key = ((IntOop *)op_stack.top())->value;
				auto iter = jump_tbl.find(key);
				if (iter == jump_tbl.end()) {
					pc = (code_begin + jump_tbl[INT_MAX]);
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) it is [lookupswitch] switch(" << key << ") {...}, didn't match... so will jump to [default]: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {
					pc = (code_begin + iter->second);
					pc -= occupied;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) it is [lookupswitch] switch(" << key << ") {...}, so will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				}
				break;
			}
			case 0xac:{		// ireturn
				// TODO: monitor...
				thread.pc = backup_pc;
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::INT);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) return an int value from stack: "<< ((IntOop*)op_stack.top())->value << std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
				return new IntOop(((IntOop *)op_stack.top())->value);	// boolean, short, char, int
			}
			case 0xad:{		// lreturn
				// TODO: monitor...
				thread.pc = backup_pc;
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::LONG);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) return an long value from stack: "<< ((LongOop*)op_stack.top())->value << std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
				return new LongOop(((LongOop *)op_stack.top())->value);	// long
			}


			case 0xae:{		// freturn
				thread.pc = backup_pc;
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::FLOAT);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) return an float value from stack: "<< ((FloatOop*)op_stack.top())->value << "f" << std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
				return new FloatOop(((FloatOop *)op_stack.top())->value);	// float
			}
			case 0xaf:{		// dreturn
				thread.pc = backup_pc;
				assert(op_stack.top()->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)op_stack.top())->get_type() == Type::DOUBLE);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) return an double value from stack: "<< ((DoubleOop*)op_stack.top())->value << "ld"<< std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
				return new DoubleOop(((DoubleOop *)op_stack.top())->value);	// double
			}


			case 0xb0:{		// areturn
				// TODO: monitor... 我在 invokeStatic 那些方法里边 monitorexit 了。相比于在这里，会有延迟。最后在解决。
				thread.pc = backup_pc;
				Oop *oop = op_stack.top();	op_stack.pop();
#ifdef DEBUG
	if (oop != 0) {
		sync_wcout{} << "(DEBUG) return an ref from stack: <class>:" << oop->get_klass()->get_name() <<  ", address: "<< std::hex << oop << std::endl;
		if (oop->get_klass()->get_name() == L"java/lang/String")	sync_wcout{} << "(DEBUG) return: [" << java_lang_string::print_stringOop((InstanceOop *)oop) << "]" << std::endl;
	}
	else
		sync_wcout{} << "(DEBUG) return an ref null from stack: <class>:" << method->return_type() <<  std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
//				assert(method->return_type() == oop->get_klass()->get_name());
				return oop;	// boolean, short, char, int
			}
			case 0xb1:{		// return
				// TODO: monitor...
				thread.pc = backup_pc;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) only return." << std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
				return nullptr;
			}
			case 0xb2:{		// getStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_field->get_klass();
				initial_clinit(new_klass, thread);
				// parse the field to RUNTIME!!
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				if (new_field->get_type() == Type::OBJECT) {
					// TODO: <clinit> of the Field object oop......
					assert(new_field->get_type_klass() != nullptr);
					initial_clinit(std::static_pointer_cast<InstanceKlass>(new_field->get_type_klass()), thread);
				}
				// get the [static Field] value and save to the stack top
				Oop *new_top;
				bool temp = new_klass->get_static_field_value(new_field, &new_top);
				assert(temp == true);
				op_stack.push(new_top);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get a static value : " << get_real_value(new_top) << " from <class>: " << new_klass->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " on to the stack." << std::endl;
#endif
				break;
			}
			case 0xb3:{		// putStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_field->get_klass();
				initial_clinit(new_klass, thread);
				// parse the field to RUNTIME!!
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				if (new_field->get_type() == Type::OBJECT) {
					// TODO: <clinit> of the Field object oop......
					assert(new_field->get_type_klass() != nullptr);
					initial_clinit(std::static_pointer_cast<InstanceKlass>(new_field->get_type_klass()), thread);
				}
				// get the stack top and save to the [static Field]
				Oop *top = op_stack.top();	op_stack.pop();
				new_klass->set_static_field_value(new_field, top);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) put a static value (unknown value type): " << get_real_value(top) << " from stack, to <class>: " << new_klass->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " and override." << std::endl;
#endif
				break;
			}
			case 0xb4:{		// getField
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// TODO: $2.8.3 的 FP_strict 浮点数转换！
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				Oop *ref = op_stack.top();	op_stack.pop();
				assert(ref->get_klass()->get_type() == ClassType::InstanceClass);		// bug !!! 有可能是没有把 this 指针放到上边。
//				std::wcout << ref->get_klass()->get_name() << " " << new_field->get_klass()->get_name() << std::endl;
//				assert(ref->get_klass() == new_field->get_klass());	// 不正确。因为左边可能是右边的子类。
				Oop *new_value;
				assert(((InstanceOop *)ref)->get_field_value(new_field, &new_value) == true);
				op_stack.push(new_value);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) get a non-static value : " << get_real_value(new_value) << " from <class>: " << ref->get_klass()->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << ", to the stack." << std::endl;
#endif
				break;
			}
			case 0xb5:{		// putField
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Fieldref);
				auto new_field = boost::any_cast<shared_ptr<Field_info>>(rt_pool[rtpool_index-1].second);
				// TODO: $2.8.3 的 FP_strict 浮点数转换！
				new_field->if_didnt_parse_then_parse();		// **important!!!**
				Oop *new_value = op_stack.top();	op_stack.pop();
				Oop *ref = op_stack.top();	op_stack.pop();
				assert(ref->get_klass()->get_type() == ClassType::InstanceClass);		// bug !!! 有可能是没有把 this 指针放到上边。
//				std::wcout << ref->get_klass()->get_name() << " " << new_field->get_klass()->get_name() << std::endl;
//				assert(ref->get_klass() == new_field->get_klass());	// 不正确。因为左边可能是右边的子类。
				((InstanceOop *)ref)->set_field_value(new_field, new_value);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) put a non-static value (unknown value type): " << get_real_value(new_value) << " from stack, to <class>: " << ref->get_klass()->get_name() << "-->" << new_field->get_name() << ":"<< new_field->get_descriptor() << " and override." << std::endl;
#endif
				break;
			}
			case 0xb6:		// invokeVirtual
			case 0xb9:{		// invokeInterface
				int rtpool_index = ((pc[1] << 8) | pc[2]);	// if is `invokeInterface`: pc[3] && pc[4] deprecated.
				if (*pc == 0xb6) {
					assert(rt_pool[rtpool_index-1].first == CONSTANT_Methodref);
				} else {
					assert(rt_pool[rtpool_index-1].first == CONSTANT_InterfaceMethodref);
				}
				auto new_method = boost::any_cast<shared_ptr<Method>>(rt_pool[rtpool_index-1].second);		// 这个方法，在我的常量池中解析的时候是按照子类同名方法优先的原则。也就是，如果最子类有同样签名的方法，父类的不会被 parse。这在 invokeStatic 和 invokeSpecial 是成立的，不过在 invokeVirtual 和 invokeInterface 中是不准的。因为后两者是动态绑定。
				// 因此，得到此方法的目的只有一个，得到方法签名。
				wstring signature = new_method->get_name() + L":" + new_method->get_descriptor();
				// TODO: 可以 verify 一下。按照 Spec
				// 1. 先 parse 参数。因为 ref 在最下边。
				int size = BytecodeEngine::parse_arg_list(new_method->get_descriptor()).size() + 1;		// don't forget `this`!!!
#ifdef DEBUG
				sync_wcout{} << "arg size: " << size << "; op_stack size: " << op_stack.size() << std::endl;	// delete
#endif
				Oop *ref;		// get ref. (this)	// same as invokespecial. but invokespecial didn't use `this` ref to get Klass.
				list<Oop *> arg_list;
				assert(op_stack.size() >= size);
				while (size > 0) {
					if (size == 1) {
						ref = op_stack.top();
					}
					arg_list.push_front(op_stack.top());
					op_stack.pop();
					size --;
				}

				// 这里用作魔改。比如，禁用 Perf 类。
				if (new_method->get_klass()->get_name() == L"sun/misc/Perf" || new_method->get_klass()->get_name() == L"sun/misc/PerfCounter") {
					if (new_method->is_void()) {
						break;
					} else if (new_method->is_return_primitive()){
						op_stack.push(new IntOop(0));
						break;
					} else {
						op_stack.push(nullptr);		// 返回值是 ByteBuffer.	// getParentDelegationTime() 返回值是 Lsun/misc/PerfCounter;
						break;
					}
				}


				// 2. get ref.
				if (ref == nullptr) {
				thread.get_stack_trace();			// delete
				}
				assert(ref != nullptr);			// `this` must not be nullptr!!!!
#ifdef DEBUG
				sync_wcout{} << "(DEBUG)";
				if (new_method->is_private()) {
					sync_wcout{} << " [private]";
				}
				if (new_method->is_static()) {
					sync_wcout{} << " [static]";
				}
				if (new_method->is_synchronized()) {
					sync_wcout{} << " [synchronized]";
				}
				sync_wcout{} << " " << ref->get_klass()->get_name() << "::" << signature << std::endl;
#endif
				shared_ptr<Method> target_method;
				if (*pc == 0xb6){
					if (ref->get_klass()->get_type() == ClassType::InstanceClass) {
						target_method = std::static_pointer_cast<InstanceKlass>(ref->get_klass())->search_vtable(signature);
					} else if (ref->get_klass()->get_type() == ClassType::TypeArrayClass || ref->get_klass()->get_type() == ClassType::ObjArrayClass) {
						target_method = new_method;		// 那么 new_method 就是那个 target_method。因为数组没有 InstanceKlass，编译器会自动把 Object 的 Method 放上来。直接调用就可以～
					} else {
						assert(false);
					}
				} else {
					assert(ref->get_klass()->get_type() == ClassType::InstanceClass);	// 接口一定是 Instance。
					target_method = std::static_pointer_cast<InstanceKlass>(ref->get_klass())->get_class_method(signature);
				}

				if (target_method == nullptr) {
					std::wcerr << "didn't find: [" << signature << "] in klass: [" << ref->get_klass()->get_name() << "]!" << std::endl;
				}
				assert(target_method != nullptr);


				// synchronize
				if (target_method->is_synchronized()) {
					ref->enter_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) synchronize obj: [" << ref << "]." << std::endl;
#endif
				}
				if (target_method->is_native()) {
					if (new_method->get_name() == L"registerNatives" && new_method->get_descriptor() == L"()V") {
#ifdef DEBUG
						sync_wcout{} << "jump off `registerNatives`." << std::endl;
#endif
						// 如果是 registerNatives 则啥也不做。因为内部已经做好了。并不打算支持 jni，仅仅打算支持 Natives.
					} else {
						shared_ptr<InstanceKlass> new_klass = new_method->get_klass();
						void *native_method = find_native(new_klass->get_name(), signature);
						// no need to add a stack frame!
						if (native_method == nullptr) {
							std::wcout << "You didn't write the [" << new_klass->get_name() << ":" << signature << "] native ";
							if (new_method->is_static()) {
								std::wcout << "[static] ";
							}
							std::wcout << "method!" << std::endl;
						}
						assert(native_method != nullptr);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) invoke a [native] method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":(this)"<< new_method->get_descriptor() << std::endl;
#endif
						arg_list.push_back(ref->get_klass()->get_mirror());		// 也要把 Klass 放进去!... 放得对不对有待考证......	// 因为是 invokeVirtual 和 invokeInterface，所以应该 ref 指向的是真的。
						arg_list.push_back((Oop *)&thread);
						// 还是要意思意思......得添一个栈帧上去......然后 pc 设为 0......
						uint8_t *backup_pc = pc;
						thread.vm_stack.push_back(StackFrame(new_method, pc, nullptr, arg_list, true));
						pc = 0;
						// execute !!
						((void (*)(list<Oop *> &))native_method)(arg_list);
						// 然后弹出并恢复 pc......
						thread.vm_stack.pop_back();
						pc = backup_pc;

						if (cur_frame.has_exception) {
							assert(arg_list.size() >= 1);
							op_stack.push(arg_list.back());
						} else if (!new_method->is_void()) {	// return value.
							assert(arg_list.size() >= 1);
							op_stack.push(arg_list.back());
#ifdef DEBUG
	sync_wcout{} << "then push invoke [native] method's return value " << op_stack.top() << " on the stack~" << std::endl;
#endif
						}
					}
				} else {
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) invoke a method: <class>: " << ref->get_klass()->get_name() << "-->" << new_method->get_name() << ":(this)"<< new_method->get_descriptor() << std::endl;
#endif
					Oop *result = thread.add_frame_and_execute(target_method, arg_list);

					if (cur_frame.has_exception) {
						op_stack.push(result);
					} else if (!target_method->is_void()) {
						op_stack.push(result);
#ifdef DEBUG
	sync_wcout{} << "then push invoke method's return value " << op_stack.top() << " on the stack~" << std::endl;
#endif
					}
				}

				// unsynchronize
				if (target_method->is_synchronized()) {
					ref->leave_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) unsynchronize obj: [" << ref << "]." << std::endl;
#endif
				}

				// **IMPORTANT** judge whether returns an Exception!!!
				if (cur_frame.has_exception/* && !new_method->is_void()*/) {
					Oop *top = op_stack.top();
					if (top != nullptr && top->get_ooptype() != OopType::_BasicTypeOop && top->get_klass()->get_type() == ClassType::InstanceClass) {
						auto klass = std::static_pointer_cast<InstanceKlass>(top->get_klass());
						auto throwable_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Throwable"));
						if (klass == throwable_klass || klass->check_parent(throwable_klass)) {		// 千万别忘了判断此 klass 是不是 throwable !!!!!
							cur_frame.has_exception = false;		// 清空标记！因为已经找到 handler 了！
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) find the last frame's exception: [" << klass->get_name() << "]. will goto exception_handler!" << std::endl;
#endif
							goto exception_handler;
						} else {
							assert(false);
						}
					} else {
						assert(false);
					}
				}

				break;
			}
			case 0xb7:		// invokeSpecial
			case 0xb8:{		// invokeStatic
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Methodref);
				auto new_method = boost::any_cast<shared_ptr<Method>>(rt_pool[rtpool_index-1].second);
				wstring signature = new_method->get_name() + L":" + new_method->get_descriptor();
				if (*pc == 0xb8) {
					assert(new_method->is_static() && !new_method->is_abstract());
				} else if (*pc == 0xb7) {
					// TODO: 可以有限制条件。
				}
				// initialize the new_class... <clinit>
				shared_ptr<InstanceKlass> new_klass = new_method->get_klass();
				initial_clinit(new_klass, thread);
#ifdef DEBUG
				sync_wcout{} << "(DEBUG)";
				if (new_method->is_private()) {
					sync_wcout{} << " [private]";
				}
				if (new_method->is_static()) {
					sync_wcout{} << " [static]";
				}
				if (new_method->is_synchronized()) {
					sync_wcout{} << " [synchronized]";
				}
				sync_wcout{} << " " << new_klass->get_name() << "::" << signature << std::endl;
#endif
				// parse arg list and push args into stack: arg_list !
				int size = BytecodeEngine::parse_arg_list(new_method->get_descriptor()).size();
				if (*pc == 0xb7) {
					size ++;		// invokeSpecial 必须加入一个 this 指针！除了 invokeStatic 之外的所有指令都要加上 this 指针！！！ ********* important ！！！！！
								// this 指针会被自动放到 op_stack 上！所以，从 op_stack 上多读一个就 ok ！！
				}
#ifdef DEBUG
				sync_wcout{} << "arg size: " << size << "; op_stack size: " << op_stack.size() << std::endl;	// delete
#endif
				list<Oop *> arg_list;
				assert(op_stack.size() >= size);
				Oop *ref = nullptr;
				while (size > 0) {
					if (size == 1 && *pc == 0xb7) {
						ref = op_stack.top();
					}
					arg_list.push_front(op_stack.top());
					op_stack.pop();
					size --;
				}

				// 这里用作魔改。比如，禁用 System.loadLibrary 方法。
				if (new_method->get_klass()->get_name() == L"java/lang/System" && new_method->get_name() == L"loadLibrary") break;
				// 这里用作魔改。比如，禁用 Perf 类。		// TODO: 这里的魔改可以去掉。是程序分支走错才走到这里的。不过留着亦可。
				if (new_method->get_klass()->get_name() == L"sun/misc/Perf" || new_method->get_klass()->get_name() == L"sun/misc/PerfCounter") {
					if (new_method->is_void()) {
						break;
					} else if (new_method->is_return_primitive()){
						op_stack.push(new IntOop(0));
						break;
					} else {
						op_stack.push(nullptr);		// 返回值是 ByteBuffer.	// getParentDelegationTime() 返回值是 Lsun/misc/PerfCounter;
						break;
					}
				}

				// synchronized:
				Oop *this_obj;
				if (new_method->is_synchronized()) {
					if (new_method->is_static()) {	// if static, lock the `mirror` of this klass.	// for 0xb8: invokeStatic
						new_method->get_klass()->get_mirror()->enter_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) synchronize klass: [" << new_method->get_klass()->get_name() << "]." << std::endl;
#endif
					} else {							// if not-static, lock this obj.					// for 0xb7: invokeSpecial
						// get the `obj` from op_stack!
						this_obj = arg_list.front();
						this_obj->enter_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) synchronize obj: [" << this_obj << "]." << std::endl;
#endif
					}
				}
				if (new_method->is_native()) {
					// TODO: 这里应该有一个 “纸上和现实中” 的问题。因为这里记录的到时候会返回函数指针，而这个指针的类型已经被完全擦除了。我们根本不知道参数的个数是多少。虽然我们能够得到
					// argument list，但是这个 argument list 又要怎么传给参数呢？这是个非常有难度的问题。
					// 好的解法，就像这里，把所有参数全都去掉，换成局部变量表和栈式虚拟机。这样的话能够避免这个问题——毕竟机器是虚拟出来的。
					// 而函数调用还是基于别人的语言基础上进行，所以根本无法在纸上进行操作了。
					// 因此，必须使用栈来解决，把所有 native 方法的参数全都换成栈。而这样，由于每个 native 方法自己知道自己有几个参数，出栈即可。
					// 返回值也一并压到栈中。
					if (new_method->get_name() == L"registerNatives" && new_method->get_descriptor() == L"()V") {
#ifdef DEBUG
						sync_wcout{} << "jump off `registerNatives`." << std::endl;
#endif
						// 如果是 registerNatives 则啥也不做。因为内部已经做好了。并不打算支持 jni，仅仅打算支持 Natives.
					} else {
#ifdef DEBUG
	if (*pc == 0xb7)
		sync_wcout{} << "(DEBUG) invoke a [native] method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":(this)"<< new_method->get_descriptor() << std::endl;
	else if (*pc == 0xb8)
		sync_wcout{} << "(DEBUG) invoke a [native] method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":"<< new_method->get_descriptor() << std::endl;
#endif
						void *native_method = find_native(new_klass->get_name(), signature);
						// no need to add a stack frame!
						if (native_method == nullptr) {
							std::wcout << "You didn't write the [" << new_klass->get_name() << ":" << signature << "] native ";
							if (new_method->is_static()) {
								std::wcout << "[static] ";
							}
							std::wcout << "method!" << std::endl;
						}
						assert(native_method != nullptr);
						if (*pc == 0xb7)
							arg_list.push_back(ref->get_klass()->get_mirror());		// 也要把 Klass 放进去!... 放得对不对有待考证......	// 因为是 invokeSpecial 可以调用父类的方法。因此从 Method 中得到 klass 应该是不安全的。而 static 应该相反。
						else
							arg_list.push_back(new_method->get_klass()->get_mirror());		// 也要把 Klass 放进去!... 放得对不对有待考证......	// 因为是 invokeSpecial 可以调用父类的方法。因此从 Method 中得到 klass 应该是不安全的。而 static 应该相反。
						arg_list.push_back((Oop *)&thread);			// 这里使用了一个小 hack。由于有的 native 方法需要使用 jvm，所以在最后边放入了一个 jvm 指针。这样就和 JNIEnv 是一样的效果了。如果要使用的话，那么直接在 native 方法中 pop_back 即可。并不影响其他的参数。	--- 后来由于加上了 Thread，所以名字改成了 thread 而已。

						// 还是要意思意思......得添一个栈帧上去......然后 pc 设为 0......
						uint8_t *backup_pc = pc;
						thread.vm_stack.push_back(StackFrame(new_method, pc, nullptr, arg_list, true));
						pc = nullptr;
						// execute !!
						((void (*)(list<Oop *> &))native_method)(arg_list);
						// 然后弹出并恢复 pc......
						thread.vm_stack.pop_back();
						pc = backup_pc;

						if (cur_frame.has_exception) {
							assert(arg_list.size() >= 1);
							op_stack.push(arg_list.back());
						} else if (!new_method->is_void()) {	// return value.
							assert(arg_list.size() >= 1);
							op_stack.push(arg_list.back());
#ifdef DEBUG
	sync_wcout{} << "then push invoke [native] method's return value " << op_stack.top() << " on the stack~" << std::endl;
#endif
						}
					}
				} else {
#ifdef DEBUG
	if (*pc == 0xb7)
		sync_wcout{} << "(DEBUG) invoke a method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":(this)"<< new_method->get_descriptor() << std::endl;
	else if (*pc == 0xb8)
		sync_wcout{} << "(DEBUG) invoke a method: <class>: " << new_klass->get_name() << "-->" << new_method->get_name() << ":"<< new_method->get_descriptor() << std::endl;
#endif
					Oop *result = thread.add_frame_and_execute(new_method, arg_list);

					if (cur_frame.has_exception) {
						op_stack.push(result);
					} else if (!new_method->is_void()) {
						op_stack.push(result);
#ifdef DEBUG
	sync_wcout{} << "then push invoke method's return value " << op_stack.top() << " on the stack~" << std::endl;
#endif
					}
				}
				// unsynchronize
				if (new_method->is_synchronized()) {
					if (new_method->is_static()) {
						new_method->get_klass()->get_mirror()->leave_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) unsynchronize klass: [" << new_method->get_klass()->get_name() << "]." << std::endl;
#endif
					} else {
						this_obj->leave_monitor();
#ifdef DEBUG
					sync_wcout{} << "(DEBUG) unsynchronize obj: [" << this_obj << "]." << std::endl;
#endif
					}
				}

				// **IMPORTANT** judge whether returns an Exception!!!
				if (cur_frame.has_exception/* && !new_method->is_void()*/) {
					Oop *top = op_stack.top();
					if (top != nullptr && top->get_ooptype() != OopType::_BasicTypeOop && top->get_klass()->get_type() == ClassType::InstanceClass) {
						auto klass = std::static_pointer_cast<InstanceKlass>(top->get_klass());
						auto throwable_klass = std::static_pointer_cast<InstanceKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Throwable"));
						if (klass == throwable_klass || klass->check_parent(throwable_klass)) {		// 千万别忘了判断此 klass 是不是 throwable !!!!!
							cur_frame.has_exception = false;		// 清空标记！因为已经找到 handler 了！
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) find the last frame's exception: [" << klass->get_name() << "]. will goto exception_handler!" << std::endl;
#endif
							goto exception_handler;
						} else {
							assert(false);
						}
					} else {
						assert(false);
					}
				}

				break;
			}
			case 0xba:{		// invokeDynamic
				assert(false);
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
				initial_clinit(real_klass, thread);
				auto oop = real_klass->new_instance();
				op_stack.push(oop);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new an object (only alloc memory): <class>: [" << klass->get_name() <<"], at address: [" << oop << "]." << std::endl;
#endif
				break;
			}
			case 0xbc:{		// newarray		// 创建普通的 basic type 数组。
				int arr_type = pc[1];
				int length = ((IntOop *)op_stack.top())->value;	op_stack.pop();
				if (length < 0) {	// TODO: 最后要全部换成异常！
					std::cerr << "array length can't be negative!!" << std::endl;
					assert(false);
				}
				wstring for_debug;
				switch (arr_type) {
					case T_BOOLEAN:
						assert(system_classmap.find(L"[Z.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[Z.class"])->new_instance(length));	// 注意！这里的 basic type array oop 内部存放的全是引用，指向 basic type oop！！千万不要搞错！需要对数组元素再一次解引用才能得到真正的值！
						for_debug = L"[Z";
						break;
					case T_CHAR:
						assert(system_classmap.find(L"[C.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[C.class"])->new_instance(length));
						for_debug = L"[C";
						break;
					case T_FLOAT:
						assert(system_classmap.find(L"[F.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[F.class"])->new_instance(length));
						for_debug = L"[F";
						break;
					case T_DOUBLE:
						assert(system_classmap.find(L"[D.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[D.class"])->new_instance(length));
						for_debug = L"[D";
						break;
					case T_BYTE:
						assert(system_classmap.find(L"[B.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[B.class"])->new_instance(length));
						for_debug = L"[B";
						break;
					case T_SHORT:
						assert(system_classmap.find(L"[S.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[S.class"])->new_instance(length));
						for_debug = L"[S";
						break;
					case T_INT:
						assert(system_classmap.find(L"[I.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[I.class"])->new_instance(length));
						for_debug = L"[I";
						break;
					case T_LONG:
						assert(system_classmap.find(L"[J.class") != system_classmap.end());
						op_stack.push(std::static_pointer_cast<TypeArrayKlass>(system_classmap[L"[J.class"])->new_instance(length));
						for_debug = L"[J";
						break;
					default:{
						assert(false);
					}
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new a basic type array ---> " << for_debug << std::endl;
#endif
				break;
			}
			case 0xbd:{		// anewarray		// 创建引用(对象)的[一维]数组。
				/**
				 * java 的数组十分神奇。在这里需要有所解释。
				 * String[][] x = new String[2][3]; 调用的是 mulanewarray. 这样初始化出来的 ArrayOop 是 [二维] 的。即 dimension == 2. 这样，内部的数组可以“参差不齐”，也就是可以放 3 个一维，但是长度任意的数组了。
				 * String[][] x = new String[2][]; x[0] = new String[2]; x[1] = new String[3]; 调用的是 anewarray. 这样初始化出来的 ArrayOop [全部] 都是 [一维] 的。即 dimension == 1.
				 * 虽然句柄的表示 String[][] 都是这个格式，但是 dimension 不同！前者，x 仅仅是一个 二维的 ArrayOop，element 是 java.lang.String。
				 * 而后者的 x 是一个 一维的 ArrayOop，element 是 [Ljava.lang.String;。
				 * 实现时千万要注意。
				 * 而且。别忘了可以有 String[] x = new String[0]。
				 * ** 产生这种表示法的根本原因在于：jvm 根本就不关心句柄是怎么表示的。它只关心的是真正的对象。句柄的表示是由编译器来关注并 parse 的！！**
				 */
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				int length = ((IntOop *)op_stack.top())->value;	op_stack.pop();
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
						op_stack.push(arr_klass->new_instance(length));
					} else {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass->get_classloader()->loadClass(L"[L" + real_klass->get_name() + L";"));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push(arr_klass->new_instance(length));
					}
				} else if (klass->get_type() == ClassType::ObjArrayClass) {	// [Ljava/lang/Class
					auto real_klass = std::static_pointer_cast<ObjArrayKlass>(klass);
					// 创建数组的数组。不过也和 if 中的逻辑相同。
					// 不过由于数组类没有设置 classloader，需要从 element 中去找。
					if (real_klass->get_element_klass()->get_classloader() == nullptr) {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[" + real_klass->get_name()));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push(arr_klass->new_instance(length));
					} else {
						auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass->get_element_klass()->get_classloader()->loadClass(L"[" + real_klass->get_name()));
						assert(arr_klass->get_type() == ClassType::ObjArrayClass);
						op_stack.push(arr_klass->new_instance(length));
					}
				} else if (klass->get_type() == ClassType::TypeArrayClass) {	// [[I --> will new an [[[I. e.g.: int[][][] = { {{1,2,3},{4,5}}, {{1},{2}}, {{1},{2}} };
					auto real_klass = std::static_pointer_cast<TypeArrayKlass>(klass);
					auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[" + real_klass->get_name()));
					assert(arr_klass->get_type() == ClassType::TypeArrayClass);
					op_stack.push(arr_klass->new_instance(length));
				} else {
					assert(false);
				}
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new an array[] of class: <class>: " << klass->get_name() << std::endl;
#endif
				break;
			}
			case 0xbe:{		// arraylength
				ArrayOop *array = (ArrayOop *)op_stack.top();	op_stack.pop();
				assert(array->get_ooptype() == OopType::_ObjArrayOop || array->get_ooptype() == OopType::_TypeArrayOop);
				op_stack.push(new IntOop(array->get_length()));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) put array: (element type) " << array->get_klass()->get_name() << " (dimension) " << array->get_dimension() << " 's length: [" << array->get_length() << "] onto the stack." << std::endl;
#endif
				break;
			}
			case 0xbf:{		// athrow
	exception_handler:
				InstanceOop *exception_obj = (InstanceOop *)op_stack.top();	op_stack.pop();
				auto excp_klass = std::static_pointer_cast<InstanceKlass>(exception_obj->get_klass());

				// backtrace ONLY this stack.
				int jump_pc = cur_frame.method->where_is_catch(pc - code_begin, excp_klass);
				if (jump_pc != 0) {	// this frame has the handler, what ever the `catch_handler` or `finally handler`.
					pc = code_begin + jump_pc - occupied;		// jump to this handler pc!
					// then clean up all the op_stack, and push `exception_obj` on it !!!
					stack<Oop *>().swap(op_stack);
					// add it on!
					op_stack.push(exception_obj);
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) [athrow] this frame has the catcher, and the pc is [" << jump_pc << "]!" << std::endl;
#endif
					break;	// go to the handler~
				} else {				// this frame cannot handle the exception!
					// make `cur_frame` lose effectiveness!
					// I think only should take this method: return an `Exception` oop, and judge at the last StackFrame's `invokeVirtual`.etc.
					// then If the last frame find an `Exception` oop has been pushed, then it will directly `GOTO` the `athrow` instruct.
					thread.pc = backup_pc;
					assert(&cur_frame == &thread.vm_stack.back());
					if (thread.vm_stack.size() == 1) {	// if cur_frame is the last...
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) [athrow] TERMINALED because of exception!!!" << std::endl;
#endif

						// 千万注意这里！！这一步此线程 exit() 执行，那么会回收所有资源！这样肯定其他线程肯定会出故障！
						// 我先简单 kill 处理了。
						// TODO: 好好看看 openjdk 怎么实现的？多线程的异常？

//						ThreadTable::kill_all_except_main_thread(pthread_self());
//						exit(-1);		// 不行......整个进程全部退出，会造成此 thread 析构，把全局的东西析构了......
						InstanceOop *thread_obj = ThreadTable::get_a_thread(pthread_self());
						assert(thread_obj != nullptr);
						auto final_method = std::static_pointer_cast<InstanceKlass>(thread_obj->get_klass())->get_class_method(L"dispatchUncaughtException:(Ljava/lang/Throwable;)V", false);
						assert(final_method != nullptr);
						thread.add_frame_and_execute(final_method, {thread_obj, exception_obj});
						pthread_exit(nullptr);		// Spec 指出，只要退出此线程即可......

					} else {
						auto iter = thread.vm_stack.rbegin();
						++iter;							// get cur_frame's prev...
						iter->has_exception = true;
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) [athrow] this frame doesn't has the catcher, or it's native method. so we should go to the last frame!" << std::endl;
	sync_wcout{} << "[Now, get out of StackFrame #" << std::dec << thread.vm_stack.size() - 1 << "]..." << std::endl;
#endif
					}
					return exception_obj;
				}

				break;
			}
			case 0xc0:		// checkcast
			case 0xc1:{		// instanceof
				// TODO: paper...
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				assert(rt_pool[rtpool_index-1].first == CONSTANT_Class);
				auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);		// constant_pool index
				// 1. first get the ref and find if it is null
				Oop *ref = op_stack.top();
				if (*pc == 0xc1) {
					op_stack.pop();
				}
				if (ref == 0) {
					// if this is 0xc0, re-push and break.
					if (*pc == 0xc1) {
						op_stack.push(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) So push 1 onto the stack." << std::endl;
#endif
					}
//					else							// bug...
//						op_stack.push(ref);		// re-push.		// "op_stack will not change if op_stack's top is null."
					else {
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ref is null. So checkcast do nothing." << std::endl;
#endif
					}
					break;
				}
				// 2. if ref is not null, judge its type
				auto ref_klass = ref->get_klass();													// op_stack top ref's klass
				bool result = check_instanceof(ref_klass, klass);
				// 3. push result
				if (result == true) {
					if (*pc == 0xc1) {
						op_stack.push(new IntOop(1));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) So push 1 onto the stack." << std::endl;
#endif
					}
					// else `checkcast` do nothing.
				} else {
					if (*pc == 0xc1) {
						op_stack.push(new IntOop(0));
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) So push 0 onto the stack." << std::endl;
#endif
					}
					else {
						// TODO: throw ClassCastException.
						assert(false);
					}
				}
				break;
			}
			case 0xc2:{		// monitorenter
				Oop *ref_value = op_stack.top();	op_stack.pop();
				// TODO: 重入？？见 Spec 关于 monitorenter 的解释...也就是递归锁 ???
				assert(ref_value != nullptr);		// TODO: 改成 NullptrException
				ref_value->enter_monitor();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) Monitor enter into obj of class:[" << ref_value->get_klass()->get_name() << "], address: [" << ref_value << "]" << std::endl;
#endif
				break;
			}
			case 0xc3:{		// monitorexit
				Oop *ref_value = op_stack.top();	op_stack.pop();
				assert(ref_value != nullptr);		// TODO: 改成 NullptrException
				// TODO: IllegalMonitorStateException...
				ref_value->leave_monitor();
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) Monitor exit from obj of class:[" << ref_value->get_klass()->get_name() << "], address: [" << ref_value << "]" << std::endl;
#endif
				break;
			}


			case 0xc5:{		// multianewarray
				int rtpool_index = ((pc[1] << 8) | pc[2]);
				int dimensions = pc[3];
				assert(dimensions > 0);		// 维度肯定要 > 0 ! 至少是 1！！
				vector<int> counts;			// 所以这里才能保证至少有 1 个元素！！
				for (int i = 0; i < dimensions; i ++) {
					Oop *count = op_stack.top();	op_stack.pop();
					assert(count->get_ooptype() == OopType::_BasicTypeOop && ((BasicTypeOop *)count)->get_type() == Type::INT);
					if (((IntOop *)count)->value < 0) {
						std::wcerr << "array length can't be negative!!" << std::endl;
						assert(false);
					}
					counts.push_back(((IntOop *)count)->value);
				}

				// lambda to recursively create a multianewarray::
				function<Oop *(shared_ptr<ArrayKlass>, int)> recursive_create_multianewarray = [&counts, &recursive_create_multianewarray](shared_ptr<ArrayKlass> arr_klass, int index) -> Oop *{
					// 1. new an multianewarray, which length is counts[index]!! ( counts[0] first )
					auto arr_obj = arr_klass->new_instance(counts[index]);
					// 2-a. if this is the last dimension(1), then return.
					if (index == counts.size() - 1) {
						assert(arr_klass->get_dimension() == 1);		// or judge with this is okay.
						return arr_obj;		// create over.
					}
					// 2-b. else, get the inner multianewarray type.
					auto inner_arr_klass = std::static_pointer_cast<ArrayKlass>(arr_klass->get_lower_dimension());
					assert(inner_arr_klass != nullptr);
					// 3. fill in every element in this `fake multiarray`, which is really one dimension... (java 的多维数组是伪的，众人皆知)
					for (int i = 0; i < arr_obj->get_length(); i ++) {
						(*arr_obj)[i] = recursive_create_multianewarray(inner_arr_klass, index + 1);
					}
					return arr_obj;
				};


				assert(rt_pool[rtpool_index-1].first == CONSTANT_Class);
				auto klass = boost::any_cast<shared_ptr<Klass>>(rt_pool[rtpool_index-1].second);
				if (klass->get_type() == ClassType::InstanceClass) {			// e.g.: java/lang/Class
					assert(false);		// TODO:没有碰到过这种情况...先注释掉......以后再补上。	// 其实我感觉规范是不是写错了？？？这根本不可能出现 InstanceKlass 的情况啊...... 毕竟下边注释也说了，给出的 rt_pool 的 klass 是正确的，不用像 anewarray 一样加上一维了......所以感觉 Spec 应该是错的？？因为根本就没有指向 InstanceKlass 的情况？
//					auto real_klass = std::static_pointer_cast<InstanceKlass>(klass);
//					// 由于仅仅是创建一维数组：所以仅仅需要把高一维的数组类加载就好。
//					shared_ptr<ObjArrayKlass> arr_klass;
//					if (real_klass->get_classloader() == nullptr) {
//						arr_klass = std::static_pointer_cast<ObjArrayKlass>(BootStrapClassLoader::get_bootstrap().loadClass(L"[L" + real_klass->get_name() + L";"));
//					} else {
//						arr_klass = std::static_pointer_cast<ObjArrayKlass>(real_klass->get_classloader()->loadClass(L"[L" + real_klass->get_name() + L";"));
//					}
//					assert(arr_klass->get_type() == ClassType::ObjArrayClass);
//					op_stack.push(arr_klass->new_instance(length));
				} else if (klass->get_type() == ClassType::ObjArrayClass) {	// e.g.: [[[Ljava/lang/Class	// 注意：这回不用再加上一维了！编译器给出的是真·数组 klass name！直接拿来用就好！
					auto arr_klass = std::static_pointer_cast<ObjArrayKlass>(klass);
					assert(arr_klass->get_type() == ClassType::ObjArrayClass);
					assert(arr_klass->get_dimension() == dimensions);	// I think must be equal here!

					op_stack.push(recursive_create_multianewarray(arr_klass, 0));

#ifdef DEBUG
	sync_wcout{} << "(DEBUG) new an multianewarray: [" << arr_klass->get_name() << "]." << std::endl;
#endif
				} else if (klass->get_type() == ClassType::TypeArrayClass) {		// int [3][4][5][6][7][8][9]
					auto arr_klass = std::static_pointer_cast<TypeArrayKlass>(klass);
					assert(arr_klass->get_type() == ClassType::TypeArrayClass);
					assert(arr_klass->get_dimension() == dimensions);	// I think must be equal here!

					op_stack.push(recursive_create_multianewarray(arr_klass, 0));
				} else {
					assert(false);
				}

				break;
			}
			case 0xc6:		// ifnull
			case 0xc7:{		// ifnonnull
				short branch_pc = ((pc[1] << 8) | pc[2]);
				Oop *ref_value = op_stack.top();	op_stack.pop();
				bool result;
				if (*pc == 0xc6) {
					result = (ref_value == nullptr) ? true : false;
				} else {
					result = (ref_value != nullptr) ? true : false;
				}
				if (result == true) {	// if not null, jump to the branch_pc.
					pc += branch_pc;		// 注意！！这里应该是 += ！ 因为 branch_pc 是根据此 ifnonnull 指令而产生的分支，基于此指令 pc 的位置！
					pc -= occupied;		// 因为最后设置了 pc += occupied 这个强制增加，因而这里强制减少。
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) ";
	if (*pc == 0xc7)sync_wcout{} << " [ifnonnull] ";		// TODO: 这里被优化掉了？？？？？无论 clang++ 还是 g++ 都？？？
	sync_wcout{} << "ref is ";
	if (*pc == 0xc7)	sync_wcout{} << "not ";
	sync_wcout{} << "null. will jump to: <bytecode>: $" << std::dec << (pc - code_begin + occupied) << std::endl;
#endif
				} else {		// if null, go next.
					// do nothing
#ifdef DEBUG
//	sync_wcout{} << "(DEBUG) ref is ";
	sync_wcout{} << "(DEBUG) ";
	if (*pc == 0xc7)sync_wcout{} << " [ifnonnull] ";
	sync_wcout{} << "ref is ";
	if (*pc == 0xc6)	sync_wcout{} << "not ";
	sync_wcout{} << "null. will go next." << std::endl;
#endif
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


#include <iostream>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <locale>
#include <vector>
#include <unordered_map>
#include <utility>
#include "class_parser.h"

using namespace std;

u1 peek1(std::ifstream & f) {	// peek u1
	return f.peek();
}

u2 peek2(std::ifstream & f) {
	u1 first = f.get();
	u1 second = f.peek();f.unget();
	return ((first << 8) + second) & 0xFFFF;
}

u4 peek4(std::ifstream & f) {
	u1 first = f.get();
	u1 second = f.get();
	u1 third = f.get();
	u1 forth = f.peek();
	f.unget();f.unget();f.unget();
	return ((first << 24) + (second << 16) + (third << 8) + forth) & 0xFFFFFFFF;
}

u1 read1(std::ifstream & f) {
	u1 result;
	f.read((char *)&result, sizeof(u1));
	return result;
}

u2 read2(std::ifstream & f) {		// normal order !!!
	u2 result;
	f.read((char *)&result, sizeof(u2));
	return htons(result);
}

u4 read4(std::ifstream & f) {
	u4 result;
	f.read((char *)&result, sizeof(u4));
	return htonl(result);
}

/*===----------- constant pool --------------===*/

// CONSTANT_CS_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_CS_info & i) {
	i.tag = read1(f);
	i.index = read2(f);
	return f;
}

// CONSTANT_FMI_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_FMI_info & i) {
	i.tag = read1(f);
	i.class_index = read2(f);
	i.name_and_type_index = read2(f);
	return f;
}

// CONSTANT_Integer_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_Integer_info & i) {
	i.tag = read1(f);
	i.bytes = read4(f);
	return f;
}
int CONSTANT_Integer_info::get_value() {
	return (int)bytes;
}

// CONSTANT_Float_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_Float_info & i) {
	i.tag = read1(f);
	i.bytes = read4(f);
	return f;
}
float CONSTANT_Float_info::get_value() {
	if (bytes == FLOAT_INFINITY)				return FLOAT_INFINITY;
	else if (bytes == FLOAT_NEGATIVE_INFINITY)	return FLOAT_NEGATIVE_INFINITY;
	else if ((bytes >= 0x7f800001 && bytes <= 0x7fffffff) || (bytes >= 0xff800001 && bytes <= 0xffffffff))	return FLOAT_NAN;
	else {
		int s = ((bytes >> 31) == 0) ? 1 : -1;
		int e = ((bytes >> 23) & 0xff);
		int m = (e == 0) ? (bytes & 0x7fffff) << 1 : (bytes & 0x7fffff) | 0x800000;		// $ 4.4.4
		return s * m * pow(2, e-150); 
	}
}

// CONSTANT_Long_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_Long_info & i) {
	i.tag = read1(f);
	i.high_bytes = read4(f);
	i.low_bytes = read4(f);
	return f;
}
long CONSTANT_Long_info::get_value() {
	return ((long)high_bytes << 32) + low_bytes;
}

// CONSTANT_Double_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_Double_info & i) {
	i.tag = read1(f);
	i.high_bytes = read4(f);
	i.low_bytes = read4(f);
	return f;
}
double CONSTANT_Double_info::get_value() {
	uint64_t bytes = ((uint64_t)high_bytes << 32) + low_bytes;	// first turns to a Long
	if(bytes == DOUBLE_INFINITY)					return DOUBLE_INFINITY;
	else if (bytes == DOUBLE_NEGATIVE_INFINITY)	return DOUBLE_NEGATIVE_INFINITY;
	else if ((bytes >= 0x7ff0000000000001L && bytes <= 0x7fffffffffffffffL) || (bytes >= 0xfff0000000000001L && bytes <= 0xffffffffffffffffL))
		return DOUBLE_NAN;
	else {
		int s = ((bytes >> 63) == 0) ? 1 : -1;
		int e = (int)((bytes >> 52) & 0x7ffL);
		long m = (e == 0) ? (bytes & 0xfffffffffffffL) << 1 : (bytes & 0xfffffffffffffL) | 0x10000000000000L;
		return s * m * pow(2, e - 1075);
	}
}

// CONSTANT_NameAndType_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_NameAndType_info & i) {
	i.tag = read1(f);
	i.name_index = read2(f);
	i.descriptor_index = read2(f);
	return f;
}

// CONSTANT_Utf8_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_Utf8_info & i) {
	i.tag = read1(f);
	i.length = read2(f);
	i.bytes = new u1[i.length];
	f.read((char*)i.bytes, sizeof(u1) * i.length);		// java utf8. need to convert it to Unicode.	// not in right order.
//		for(int pos = 0; pos < i.length; pos ++) {		// save to u1[], no need to htonl(). only save to u2[]/u4[] should convert. because of little endian is "reverse to save"
//			i.bytes[pos] = read1(f);
//		}
	return f;
}
bool CONSTANT_Utf8_info::test_bit_1(int position, int bit_pos) {	// test some bit is 1 or not ?
	u1 target = bytes[position];
	int real_pos = BIT_NUM - position - 1;
	assert(real_pos >= 0);
	return (target >> real_pos) & 1;
}
bool CONSTANT_Utf8_info::is_first_type(int position) {		// $ 4.4.7
	assert(position + 1 <= length);
	return (bytes[position] & 0x80) == 0;
}
bool CONSTANT_Utf8_info::is_second_type(int position) {
	assert(position + 2 <= length);
	return (bytes[position] & 0xE0) == 0xC0 && (bytes[position+1] & 0xC0) == 0x80;
}
bool CONSTANT_Utf8_info::is_third_type(int position) {
	assert(position + 3 <= length);
	return (bytes[position] & 0xF0) == 0xE0 && (bytes[position+1] & 0xC0) == 0x80 && (bytes[position+2] & 0xC0) == 0x80;
}
bool CONSTANT_Utf8_info::is_forth_type(int position) {
	assert(position + 6 <= length);
	return (bytes[position] == 0xED) && (bytes[position+1] & 0xF0) == 0xA0 && (bytes[position+2] & 0xC0) == 0x80 &&
			(bytes[position+3] == 0xED) && (bytes[position+4] & 0xF0) == 0xB0 && (bytes[position+5] & 0xC0) == 0x80;
}
u2 CONSTANT_Utf8_info::cal_first_type(int position) {
	return bytes[position];
}
u2 CONSTANT_Utf8_info::cal_second_type(int position) {
	return ((bytes[position] & 0x1f) << 6) + (bytes[position+1] & 0x3f);
}
u2 CONSTANT_Utf8_info::cal_third_type(int position) {
	return ((bytes[position] & 0xf) << 12) + ((bytes[position+1] & 0x3f) << 6) + (bytes[position+2] & 0x3f);
}
u2 CONSTANT_Utf8_info::cal_forth_type(int position) {
	return 0x10000 + ((bytes[position+1] & 0x0f) << 16) + ((bytes[position+2] & 0x3f) << 10)
					+ ((bytes[position+4] & 0x0f) << 6) + (bytes[position+5] & 0x3f);
}
std::wstring CONSTANT_Utf8_info::convert_to_Unicode() {
	if (str == L"") {
		vector<u2> convert_buf;
		for(int pos = 0; pos < length; ) {
			if(is_first_type(pos)) {
				convert_buf.push_back(cal_first_type(pos));
				pos += 1;
			} else if (is_second_type(pos)) {
				convert_buf.push_back(cal_second_type(pos));
				pos += 2;
			} else if (is_third_type(pos)) {
				convert_buf.push_back(cal_third_type(pos));
				pos += 3;
			} else if (is_forth_type(pos)) {
				convert_buf.push_back(cal_forth_type(pos));
				pos += 6;
			} else {
				std::cerr << "can't get here..." << std::endl;
				assert(false);
			}
		}
		str = wstring(convert_buf.begin(), convert_buf.end());	// make a cache to be efficient
	}
	return str;
}
CONSTANT_Utf8_info::~CONSTANT_Utf8_info() { if (bytes != nullptr)	delete bytes; }

// CONSTANT_MethodHandle_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_MethodHandle_info & i) {
	i.tag = read1(f);
	i.reference_kind = read1(f);
	i.reference_index = read2(f);
	return f;
}

// CONSTANT_MethodType_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_MethodType_info & i) {
	i.tag = read1(f);
	i.descriptor_index = read2(f);
	return f;
}

// CONSTANT_InvokeDynamic_info
std::ifstream & operator >> (std::ifstream & f, CONSTANT_InvokeDynamic_info & i) {
	i.tag = read1(f);
	i.bootstrap_method_attr_index = read2(f);
	i.name_and_type_index = read2(f);
	return f;
}


/*===-----------  DEBUG CONSTANT POOL -----------===*/

void print_constant_pool(cp_info **bufs, int length) {	// constant pool length
	for(int i = 0; i < length; i ++) {
		switch (bufs[i]->tag) {
			case CONSTANT_Class:
			case CONSTANT_String:{
				CONSTANT_CS_info* target = (CONSTANT_CS_info*)bufs[i];
				if (target->tag == CONSTANT_Class)
					printf("(DEBUG) #%4d = Class %15s #%d\n", i+1, "", target->index);
				else
					printf("(DEBUG) #%4d = String %14s #%d\n", i+1, "", target->index);
				break;				
			}
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:{
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
				printf("(DEBUG) #%4d = Integer %13s %di\n", i+1, "", target->get_value());
				break;
			}
			case CONSTANT_Float:{
				CONSTANT_Float_info* target = (CONSTANT_Float_info*)bufs[i];
				float result = target->get_value();
				if (result == FLOAT_INFINITY)
					printf("(DEBUG) #%4d = Float %15s Infinityf\n", i+1, "");
				else if (result == FLOAT_NEGATIVE_INFINITY)
					printf("(DEBUG) #%4d = Float %15s -Infinityf\n", i+1, "");
				else if (result == FLOAT_NAN)
					printf("(DEBUG) #%4d = Float %15s NaNf\n", i+1, "");
				else 
					printf("(DEBUG) #%4d = Float %15s %ff\n", i+1, "", result);
				break;
			}
			case CONSTANT_Long:{
				CONSTANT_Long_info* target = (CONSTANT_Long_info*)bufs[i];
				printf("(DEBUG) #%4d = Long %16s %ldl\n", i+1, "", target->get_value());
				i ++;
				break;
			}
			case CONSTANT_Double:{
				CONSTANT_Double_info* target = (CONSTANT_Double_info*)bufs[i];
				double result = target->get_value();
				if (result == DOUBLE_INFINITY)
					printf("(DEBUG) #%4d = Double %14s Infinityd\n", i+1, "");
				else if (result == DOUBLE_NEGATIVE_INFINITY)
					printf("(DEBUG) #%4d = Double %14s -Infinityd\n", i+1, "");
				else if (result == DOUBLE_NAN)
					printf("(DEBUG) #%4d = Double %14s NaNd\n", i+1, "");
				else 
					printf("(DEBUG) #%4d = Double %14s %lfd\n", i+1, "", result);
				i ++;
				break;
			}
			case CONSTANT_NameAndType:{
				CONSTANT_NameAndType_info* target = (CONSTANT_NameAndType_info*)bufs[i];
				printf("(DEBUG) #%4d = NameAndType %9s #%d.#%d\n", i+1, "", target->name_index, target->descriptor_index);
				break;
			}
			case CONSTANT_Utf8:{
				CONSTANT_Utf8_info* target = (CONSTANT_Utf8_info*)bufs[i];
				printf("(DEBUG) #%4d = Utf8 %16s ", i+1, "");
				std::wcout.imbue(locale(""));
				std::wcout << target->convert_to_Unicode() << std::endl;
				break;
			}
			default:{
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

/*===------------ Field && Method --------------===*/

// attribute_info
std::ifstream & operator >> (std::ifstream & f, attribute_info & i) {
	i.attribute_name_index = read2(f);
	i.attribute_length = read4(f);
	return f;
}

// field_info
//std::ifstream & operator >> (std::ifstream & f, field_info & i) {
void field_info::fill(std::ifstream & f, cp_info **constant_pool) {
	access_flags = read2(f);
	name_index = read2(f);
	descriptor_index = read2(f);
	attributes_count = read2(f);
	if (attributes_count != 0)		// !!!!! attention !!!!!
		attributes = new attribute_info*[attributes_count];
	for(int pos = 0; pos < attributes_count; pos ++) {
		attributes[pos] = new_attribute(f, constant_pool);
	}
}
field_info::~field_info() { 
	if(attributes != nullptr) {
		for(int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes; 
	}
}

// method_info
//std::ifstream & operator >> (std::ifstream & f, method_info & i) {
void method_info::fill(std::ifstream & f, cp_info **constant_pool) {
	access_flags = read2(f);
	name_index = read2(f);
	descriptor_index = read2(f);
	attributes_count = read2(f);
	if (attributes_count != 0)
		attributes = new attribute_info*[attributes_count];
	for(int pos = 0; pos < attributes_count; pos ++) {
		attributes[pos] = new_attribute(f, constant_pool);
	}
}
method_info::~method_info() { 
	if(attributes != nullptr) {
		for(int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes; 
	}
}

/*===----------- Attributes ---------------===*/

// aux hashmap
std::unordered_map<std::wstring, int> attribute_table {
	{L"ConstantValue", 0},
	{L"Code", 1},	
	{L"StackMapTable", 2},	
	{L"Exceptions", 3},	
	{L"InnerClasses", 4},	
	{L"EnclosingMethod", 5},	
	{L"Synthetic", 6},	
	{L"Signature", 7},	
	{L"SourceFile", 8},	
	{L"SourceDebugExtension", 9},	
	{L"LineNumberTable", 10},	
	{L"LocalVariableTable", 11},	
	{L"LocalVariableTypeTable", 12},	
	{L"Deprecated", 13},	
	{L"RuntimeVisibleAnnotations", 14},	
	{L"RuntimeInvisibleAnnotations", 15},	
	{L"RuntimeVisibleParameterAnnotations", 16},	
	{L"RuntimeInvisibleParameterAnnotations", 17},	
	{L"RuntimeVisibleTypeAnnotations", 18},
	{L"RuntimeInvisibleTypeAnnotations", 19},
	{L"AnnotationDefault", 20},	
	{L"BootstrapMethods", 21},	
	{L"MethodParameters", 22},	
};


// aux function
int peek_attribute(u2 attribute_name_index, cp_info **constant_pool) {	// look ahead ifstream to see which attribute the next is.
	assert(constant_pool[attribute_name_index-1]->tag == CONSTANT_Utf8);		// must be a UTF8 tag.
	std::wstring str = ((CONSTANT_Utf8_info *)constant_pool[attribute_name_index-1])->convert_to_Unicode();
	if(attribute_table.find(str) != attribute_table.end()) {
		return attribute_table[str];
	} else {
		std::cerr << "my jvm don't alloc a new attribute, because we don't have a compiler~" << std::endl;
		assert(false);
		return -1;
	}
}

// ConstantValue_attribute
std::ifstream & operator >> (std::ifstream & f, ConstantValue_attribute & i) {
	f >> *((attribute_info *)&i);		// force to invoke [attribute_info class]'s [friend operator >> ].
	i.constantvalue_index = read2(f);
//	i.attribute_length = 2;		// ??? should be input or not ??????
	return f;
}

// Code_attribute
std::ifstream & operator >> (std::ifstream & f, Code_attribute::exception_table_t & i) {
	i.start_pc = read2(f);
	i.end_pc = read2(f);
	i.handler_pc = read2(f);
	i.catch_type = read2(f);
	return f;
}
void Code_attribute::fill(std::ifstream & f, cp_info **constant_pool) {
//	friend std::ifstream & operator >> (std::ifstream & f, Code_attribute & i) {		// because we need constant pool. so operator >> 's arguments are not enough. so we should cancel the operator >> and subsitute it with a funciton with 3 argument: this, f, and constant pool....
	f >> *((attribute_info *)this);
	max_stack = read2(f);
	max_locals = read2(f);
	code_length = read4(f);
	if (code_length != 0) {
		code = new u1[code_length];
		f.read((char *)code, sizeof(u1) * code_length);
	}
	exception_table_length = read2(f);
	if (exception_table_length != 0) {
		exception_table = new Code_attribute::exception_table_t[exception_table_length];
		for(int pos = 0; pos < exception_table_length; pos ++) {
			f >> exception_table[pos];
		}
	}
	attributes_count = read2(f);
	if (attributes_count != 0)
		attributes = new attribute_info*[attributes_count];
	for(int pos = 0; pos < attributes_count; pos ++) {
		attribute_info* new_attribute(std::ifstream &, cp_info **);
		attributes[pos] = new_attribute(f, constant_pool);
	}
}
Code_attribute::~Code_attribute() {
	if(code != nullptr) delete code; 
	if(exception_table != nullptr) delete exception_table; 
	if(attributes != nullptr) {
		for(int i = 0; i < attributes_count; i ++) {
			delete attributes[i];
		}
		delete[] attributes;
	} 
}

// StackMapTable Attributes
#define ITEM_Top				0
#define ITEM_Integer			1
#define ITEM_Float				2
#define ITEM_Double				3
#define ITEM_Long				4
#define ITEM_Null				5
#define ITEM_UninitializedThis	6
#define ITEM_Object				7
#define ITEM_Uninitialized		8

// StackMapTable_attribute
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::verification_type_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Top_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Integer_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Float_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Double_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Long_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Null_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::UninitializedThis_variable_info & i) {
	i.tag = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Object_variable_info & i) {
	i.tag = read1(f);
	i.cpool_index = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::Uninitialized_variable_info & i) {
	i.tag = read1(f);
	i.offset = read2(f);
	return f;
}
	
// StackMapTable aux function
static StackMapTable_attribute::verification_type_info* create_verification_type(std::ifstream & f) {		// static 的函数竟然能够直接访问内部非 static 的类......emmmmm。
	u1 verification_tag = peek1(f);
	
	switch (verification_tag) {
		case ITEM_Top:{
			auto *tvi = new StackMapTable_attribute::Top_variable_info;
			f >> (*tvi);
			return tvi;
		}
		case ITEM_Integer:{
			auto *ivi = new StackMapTable_attribute::Integer_variable_info;
			f >> (*ivi);
			return ivi;
		}
		case ITEM_Float:{
			auto *fvi = new StackMapTable_attribute::Float_variable_info;
			f >> (*fvi);
			return fvi;
		}
		case ITEM_Double:{
			auto *dvi = new StackMapTable_attribute::Double_variable_info;
			f >> (*dvi);
			return dvi;
		}
		case ITEM_Long:{
			auto *lvi = new StackMapTable_attribute::Long_variable_info;
			f >> (*lvi);
			return lvi;
		}
		case ITEM_Null:{
			auto *nvi = new StackMapTable_attribute::Null_variable_info;
			f >> (*nvi);
			return nvi;
		}
		case ITEM_UninitializedThis:{
			auto *utvi = new StackMapTable_attribute::UninitializedThis_variable_info;
			f >> (*utvi);
			return utvi;
		}
		case ITEM_Object:{
			auto *ovi = new StackMapTable_attribute::Object_variable_info;
			f >> (*ovi);
			return ovi;
		}
		case ITEM_Uninitialized:{
			auto *uvi = new StackMapTable_attribute::Uninitialized_variable_info;
			f >> (*uvi);
			return uvi;
		}
		default:{
			std::cerr << "can't get here! in create_verification_type() " << std::endl;
			assert(false);
		}
	}
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::stack_map_frame & i) {
	i.frame_type = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_frame & i) {
	i.frame_type = read1(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_locals_1_stack_item_frame & i) {
	i.frame_type = read1(f);
	i.stack[0] = create_verification_type(f);
	return f;
}
StackMapTable_attribute::same_locals_1_stack_item_frame::~same_locals_1_stack_item_frame() { delete stack[0]; }
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_locals_1_stack_item_frame_extended & i) {
	i.frame_type = read1(f);
	i.offset_delta = read2(f);
	i.stack[0] = create_verification_type(f);
	return f;
}
StackMapTable_attribute::same_locals_1_stack_item_frame_extended::~same_locals_1_stack_item_frame_extended() { delete stack[0]; }
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::chop_frame & i) {
	i.frame_type = read1(f);
	i.offset_delta = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::same_frame_extended & i) {
	i.frame_type = read1(f);
	i.offset_delta = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::append_frame & i) {
	i.frame_type = read1(f);
	i.offset_delta = read2(f);
	if (i.frame_type - 251 != 0)
		i.locals = new StackMapTable_attribute::verification_type_info*[i.frame_type-251];
	for(int pos = 0; pos < i.frame_type-251; pos ++) {
		i.locals[pos] = create_verification_type(f);
	}
	return f;
}
StackMapTable_attribute::append_frame::~append_frame() {
	if(locals != nullptr) {
		for(int i = 0; i < frame_type-251; i ++) {
			delete locals[i];
		}
		delete[] locals;
	}
}
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute::full_frame & i) {
	i.frame_type = read1(f);
	i.offset_delta = read2(f);
	i.number_of_locals = read2(f);
	if (i.number_of_locals != 0)
		i.locals = new StackMapTable_attribute::verification_type_info*[i.number_of_locals];
	for(int pos = 0; pos < i.number_of_locals; pos ++) {
		i.locals[pos] = create_verification_type(f);
	}
	i.number_of_stack_items = read2(f);
	if (i.number_of_stack_items != 0)
		i.stack = new StackMapTable_attribute::verification_type_info*[i.number_of_stack_items];
	for(int pos = 0; pos < i.number_of_stack_items; pos ++) {
		i.stack[pos] = create_verification_type(f);
	}
	return f;
}
StackMapTable_attribute::full_frame::~full_frame() {
	if(locals != nullptr) {
		for(int i = 0; i < number_of_locals; i ++) {
			delete locals[i];
		}
		delete[] locals;
	}
	if(stack != nullptr) {
		for(int i = 0; i < number_of_stack_items; i ++) {
			delete stack[i];
		}
		delete[] stack;
	}
}
	
// StackMapTable aux function
static StackMapTable_attribute::stack_map_frame* peek_stackmaptable_frame(std::ifstream & f) {
	u1 frame_type = peek1(f);
	if (frame_type >= 0 && frame_type <= 63) {
		auto *frame = new StackMapTable_attribute::same_frame;
		f >> *frame;
		return frame;
	} else if (frame_type >= 64 && frame_type <= 127) {
		auto *frame = new StackMapTable_attribute::same_locals_1_stack_item_frame;
		f >> *frame;
		return frame;
	} else if (frame_type == 247) {
		auto *frame = new StackMapTable_attribute::same_locals_1_stack_item_frame_extended;
		f >> *frame;
		return frame;
	} else if (frame_type >= 248 && frame_type <= 250) {
		auto *frame = new StackMapTable_attribute::chop_frame;
		f >> *frame;
		return frame;
	} else if (frame_type == 251) {
		auto *frame = new StackMapTable_attribute::same_frame_extended;
		f >> *frame;
		return frame;
	} else if (frame_type >= 252 && frame_type <= 254) {
		auto *frame = new StackMapTable_attribute::append_frame;
		f >> *frame;
		return frame;
	} else if (frame_type == 255) {
		auto *frame = new StackMapTable_attribute::full_frame;
		f >> *frame;
		return frame;
	} else {
		std::cerr << "can't get here! in peek_stackmaptable_frame()" << std::endl;
		assert(false);
	}
}
	
// per se ---- StackMapTable
std::ifstream & operator >> (std::ifstream & f, StackMapTable_attribute & i) {
	f >> *((attribute_info *)&i);
	i.number_of_entries = read2(f);
	if (i.number_of_entries != 0)
		i.entries = new StackMapTable_attribute::stack_map_frame*[i.number_of_entries];
	for(int pos = 0; pos < i.number_of_entries; pos ++) {
		i.entries[pos] = peek_stackmaptable_frame(f);
	}
	return f;
}
StackMapTable_attribute::~StackMapTable_attribute() {
	if (entries != nullptr)	{
		for(int i = 0; i < number_of_entries; i ++) {
			delete entries[i];
		}
		delete[] entries;
	}
}

// Exceptions_attribute
std::ifstream & operator >> (std::ifstream & f, Exceptions_attribute & i) {
	f >> *((attribute_info *)&i);
	i.number_of_exceptions = read2(f);
	if (i.number_of_exceptions != 0)
		i.exception_index_table = new u2[i.number_of_exceptions];
	for(int pos = 0; pos < i.number_of_exceptions; pos ++) {
		i.exception_index_table[pos] = read2(f);
	}
	return f;
}
Exceptions_attribute::~Exceptions_attribute() { delete[] exception_index_table; }

// InnerClasses_attribute
std::ifstream & operator >> (std::ifstream & f, InnerClasses_attribute::classes_t & i) {
	i.inner_class_info_index = read2(f);
	i.outer_class_info_index = read2(f);
	i.inner_name_index = read2(f);
	i.inner_class_access_flags = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, InnerClasses_attribute & i) {
	f >> *((attribute_info *)&i);
	i.number_of_classes = read2(f);
	// struct classes_t has no polymorphism. so dont need to define: `struct classes_t **classes`, use `struct classes_t *class` is okay.
	if (i.number_of_classes != 0)
		i.classes = new InnerClasses_attribute::classes_t[i.number_of_classes];
	for(int pos = 0; pos < i.number_of_classes; pos ++) {
		f >> i.classes[pos];
	}
	return f;
}
InnerClasses_attribute::~InnerClasses_attribute() { delete[] classes; }

// EnclosingMethod_attribute
std::ifstream & operator >> (std::ifstream & f, EnclosingMethod_attribute & i) {
	f >> *((attribute_info *)&i);
	i.class_index = read2(f);
	i.method_index = read2(f);
	return f;
}

// Synthetic_attribute
std::ifstream & operator >> (std::ifstream & f, Synthetic_attribute & i) {
	f >> *((attribute_info *)&i);
	return f;
}

// Signature_attribute
std::ifstream & operator >> (std::ifstream & f, Signature_attribute & i) {
	f >> *((attribute_info *)&i);
	i.signature_index = read2(f);
	return f;
}

// SourceFile_attribute
std::ifstream & operator >> (std::ifstream & f, SourceFile_attribute & i) {
	f >> *((attribute_info *)&i);
	i.sourcefile_index = read2(f);
	return f;
}

// SourceDebugExtension_attribute
std::ifstream & operator >> (std::ifstream & f, SourceDebugExtension_attribute & i) {
	f >> *((attribute_info *)&i);
	if (i.attribute_length != 0)
		i.debug_extension = new u1[i.attribute_length];
	f.read((char *)i.debug_extension, sizeof(u1) * i.attribute_length);
	return f;
}
SourceDebugExtension_attribute::~SourceDebugExtension_attribute() { delete[] debug_extension; }

// LineNumberTable_attribute
std::ifstream & operator >> (std::ifstream & f, LineNumberTable_attribute::line_number_table_t & i) {
	i.start_pc = read2(f);
	i.line_number = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, LineNumberTable_attribute & i) {
	f >> *((attribute_info *)&i);
	i.line_number_table_length = read2(f);
	if (i.line_number_table_length != 0)
		i.line_number_table = new LineNumberTable_attribute::line_number_table_t[i.line_number_table_length];
	for(int pos = 0; pos < i.line_number_table_length; pos ++) {
		f >> i.line_number_table[pos];
	}
	return f;
}
LineNumberTable_attribute::~LineNumberTable_attribute() { delete[] line_number_table; }

// LocalVariableTable_attribute
std::ifstream & operator >> (std::ifstream & f, LocalVariableTable_attribute::local_variable_table_t & i) {
	i.start_pc = read2(f);
	i.length = read2(f);
	i.name_index = read2(f);
	i.descriptor_index = read2(f);
	i.index = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, LocalVariableTable_attribute & i) {
	f >> *((attribute_info *)&i);
	i.local_variable_table_length = read2(f);
	if (i.local_variable_table_length != 0)
		i.local_variable_table = new LocalVariableTable_attribute::local_variable_table_t[i.local_variable_table_length];
	for(int pos = 0; pos < i.local_variable_table_length; pos ++) {
		f >> i.local_variable_table[pos];
	}
	return f;
}
LocalVariableTable_attribute::~LocalVariableTable_attribute() { delete[] local_variable_table; }

// LocalVariableTypeTable_attribute
std::ifstream & operator >> (std::ifstream & f, LocalVariableTypeTable_attribute::local_variable_type_table_t & i) {
	i.start_pc = read2(f);
	i.length = read2(f);
	i.name_index = read2(f);
	i.signature_index = read2(f);
	i.index = read2(f);
	return f;
}
std::ifstream & operator >> (std::ifstream & f, LocalVariableTypeTable_attribute & i) {
	f >> *((attribute_info *)&i);
	i.local_variable_type_table_length = read2(f);
	if (i.local_variable_type_table_length != 0)
		i.local_variable_type_table = new LocalVariableTypeTable_attribute::local_variable_type_table_t[i.local_variable_type_table_length];
	for(int pos = 0; pos < i.local_variable_type_table_length; pos ++) {
		f >> i.local_variable_type_table[pos];
	}
	return f;
}
LocalVariableTypeTable_attribute::~LocalVariableTypeTable_attribute() { delete[] local_variable_type_table; }

// Deprecated_attribute
std::ifstream & operator >> (std::ifstream & f, Deprecated_attribute & i) {
	f >> *((attribute_info *)&i);
	return f;
}

// element_value

std::ifstream & operator >> (std::ifstream & f, const_value_t & i) {
	i.const_value_index = read2(f);
	return f;	
}

std::ifstream & operator >> (std::ifstream & f, enum_const_value_t & i) {
	i.type_name_index = read2(f);
	i.const_name_index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, class_info_t & i) {
	i.class_info_index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, array_value_t & i) {
	i.num_values = read2(f);
	if (i.num_values != 0)		// 这里写成了 i.values...... 本来就是 nullptr 是 0 ....... 结果调了一个小时...... 一直显示在下边 f >> i.values[pos] 进入函数中的第一行出错...... 唉（ 还以为是标准库错了（逃 我真是个白痴（打脸
		i.values = new element_value[i.num_values];
	for(int pos = 0; pos < i.num_values; pos ++) {
		f >> i.values[pos];
	}
	return f;
}
array_value_t::~array_value_t() { delete[] values; }		// 本来没有问题却说 array_value_t 内部的构造函数被删除了。但我估计应该是 annotation 不完全类型的原因。明天重构，把 .h 和 .c 分开来。

std::ifstream & operator >> (std::ifstream & f, element_value & i) {
	i.tag = read1(f);
	switch ((char)i.tag) {
		case 'B':
		case 'C':
		case 'D':
		case 'F':
		case 'I':
		case 'J':
		case 'S':
		case 'Z':
		case 's':{
			i.value = new const_value_t;
			f >> *(const_value_t *)i.value;
			break;
		}
		case 'e':{
			i.value = new enum_const_value_t;
			f >> *(enum_const_value_t *)i.value;
			break;
		}
		case 'c':{
			i.value = new class_info_t;
			f >> *(class_info_t *)i.value;
			break;
		}
		case '@':{
			i.value = new annotation;
			f >> *(annotation *)i.value;
			break;
		}
		case '[':{
			i.value = new array_value_t;
			f >> *(array_value_t *)i.value;
			break;
		}
		default:{
			std::cerr << "can't get here. in element_value." << std::endl;
			assert(false);
		}
	}
	return f;
}
element_value::~element_value() { delete value; }
	
// annotation
std::ifstream & operator >> (std::ifstream & f, annotation::element_value_pairs_t & i) {
	i.element_name_index = read2(f);
	f >> i.value;
	return f;
}

std::ifstream & operator >> (std::ifstream & f, annotation & i) {
	i.type_index = read2(f);
	i.num_element_value_pairs = read2(f);
	if (i.num_element_value_pairs != 0)
		i.element_value_pairs = new annotation::element_value_pairs_t[i.num_element_value_pairs];
	for(int pos = 0; pos < i.num_element_value_pairs; pos ++) {
		f >> i.element_value_pairs[pos];
	}
	return f;
}

annotation::~annotation() { delete[] element_value_pairs; }

// type_annotation
std::ifstream & operator >> (std::ifstream & f, type_annotation::type_parameter_target & i) {
	i.type_parameter_index = read1(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::supertype_target & i) {
	i.supertype_index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::type_parameter_bound_target & i) {
	i.type_parameter_index = read1(f);
	i.bound_index = read1(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::empty_target & i) {
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::formal_parameter_target & i) {
	i.formal_parameter_index = read1(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::throws_target & i) {
	i.throws_type_index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::localvar_target::table_t & i) {
	i.start_pc = read2(f);
	i.length = read2(f);
	i.index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::localvar_target & i) {
	i.table_length = read2(f);
	if (i.table_length != 0)
		i.table = new type_annotation::localvar_target::table_t[i.table_length];
	for(int pos = 0; pos < i.table_length; pos ++) {
		f >> i.table[pos];
	}
	return f;
}

type_annotation::localvar_target::~localvar_target()	{ delete[] table; }

std::ifstream & operator >> (std::ifstream & f, type_annotation::catch_target & i) {
	i.exception_table_index = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::offset_target & i) {
	i.offset = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::type_argument_target & i) {
	i.offset = read2(f);
	i.type_argument_index = read1(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::type_path::path_t & i) {
	i.type_path_kind = read1(f);
	i.type_argument_index = read1(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation::type_path & i) {
	i.path_length = read1(f);
	if (i.path_length != 0)
		i.path = new type_annotation::type_path::path_t[i.path_length];
	for(int pos = 0; pos < i.path_length; pos ++) {
		f >> i.path[pos];
	}
	return f;
}

type_annotation::type_path::~type_path() { delete[] path; }

std::ifstream & operator >> (std::ifstream & f, type_annotation::element_value_pairs_t & i) {
	i.element_name_index = read2(f);
	f >> i.value;
	return f;
}

std::ifstream & operator >> (std::ifstream & f, type_annotation & i) {
	i.target_type = read1(f);
	if (i.target_type == 0x00 || i.target_type == 0x01) {
		auto *result = new type_annotation::type_argument_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x10) {
		auto *result = new type_annotation::supertype_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x11 || i.target_type == 0x12) {
		auto *result = new type_annotation::type_parameter_bound_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x13 || i.target_type == 0x14 || i.target_type == 0x15) {
		auto *result = new type_annotation::empty_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x16) {
		auto *result = new type_annotation::formal_parameter_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x17) {
		auto *result = new type_annotation::throws_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x40 || i.target_type == 0x41) {
		auto *result = new type_annotation::localvar_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x42) {
		auto *result = new type_annotation::catch_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x43 || i.target_type == 0x44 || i.target_type == 0x45 || i.target_type == 0x46) {
		auto *result = new type_annotation::offset_target;
		f >> *result;
		i.target_info = result;
	} else if (i.target_type == 0x47 || i.target_type == 0x48 || i.target_type == 0x49 || i.target_type == 0x4A || i.target_type == 0x4B) {
		auto *result = new type_annotation::type_argument_target;
		f >> *result;
		i.target_info = result;
	} else {
		std::cerr << "can't get here! in type_annotation struct." << std::endl;
		assert(false);
	}
	f >> i.target_path;
	i.type_index = read2(f);
	i.num_element_value_pairs = read2(f);
	if (i.num_element_value_pairs != 0)
		i.element_value_pairs = new type_annotation::element_value_pairs_t[i.num_element_value_pairs];
	for(int pos = 0; pos < i.num_element_value_pairs; pos ++) {
		f >> i.element_value_pairs[pos];
	}
	return f;
}

type_annotation::~type_annotation() {
	delete target_info;
	delete[] element_value_pairs;
}

std::ifstream & operator >> (std::ifstream & f, parameter_annotations_t & i) {
	i.num_annotations = read2(f);
	if (i.num_annotations != 0)
		i.annotations = new annotation[i.num_annotations];
	for(int pos = 0; pos < i.num_annotations; pos ++) {
		f >> i.annotations[pos];
	}
	return f;
}

parameter_annotations_t::~parameter_annotations_t() { delete[] annotations; }

std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	f >> i.parameter_annotations;
	return f;
}

std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	f >> i.parameter_annotations;
	return f;
}

std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleParameterAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	i.num_parameters = read1(f);
	if (i.num_parameters != 0)
		i.parameter_annotations = new parameter_annotations_t[i.num_parameters];
	for(int pos = 0; pos < i.num_parameters; pos ++) {
		f >> i.parameter_annotations[pos];
	}
	return f;
}
RuntimeVisibleParameterAnnotations_attribute::~RuntimeVisibleParameterAnnotations_attribute() { delete[] parameter_annotations; }

std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleParameterAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	i.num_parameters = read1(f);
	if (i.num_parameters != 0)	
		i.parameter_annotations = new parameter_annotations_t[i.num_parameters];
	for(int pos = 0; pos < i.num_parameters; pos ++) {
		f >> i.parameter_annotations[pos];
	}
	return f;
}
RuntimeInvisibleParameterAnnotations_attribute::~RuntimeInvisibleParameterAnnotations_attribute() { delete[] parameter_annotations; }

std::ifstream & operator >> (std::ifstream & f, RuntimeVisibleTypeAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	i.num_annotations = read2(f);
	if (i.num_annotations != 0)
		i.annotations = new type_annotation[i.num_annotations];
	for(int pos = 0; pos < i.num_annotations; pos ++) {
		f >> i.annotations[pos];
	}
	return f;
}
RuntimeVisibleTypeAnnotations_attribute::~RuntimeVisibleTypeAnnotations_attribute() { delete[] annotations; }

std::ifstream & operator >> (std::ifstream & f, RuntimeInvisibleTypeAnnotations_attribute & i) {
	f >> *((attribute_info *)&i);
	i.num_annotations = read2(f);
	if (i.num_annotations != 0)
		i.annotations = new type_annotation[i.num_annotations];
	for(int pos = 0; pos < i.num_annotations; pos ++) {
		f >> i.annotations[pos];
	}
	return f;
}
RuntimeInvisibleTypeAnnotations_attribute::~RuntimeInvisibleTypeAnnotations_attribute() { delete[] annotations; }

std::ifstream & operator >> (std::ifstream & f, AnnotationDefault_attribute & i) {
	f >> *((attribute_info *)&i);
	f >> i.default_value;
	return f;
}

std::ifstream & operator >> (std::ifstream & f, BootstrapMethods_attribute::bootstrap_methods_t & i) {
	i.bootstrap_method_ref = read2(f);
	i.num_bootstrap_arguments = read2(f);
	if (i.num_bootstrap_arguments != 0)
		i.bootstrap_arguments = new u2[i.num_bootstrap_arguments];
	for(int pos = 0; pos < i.num_bootstrap_arguments; pos ++) {
		i.bootstrap_arguments[pos] = read2(f);
	}
	return f;
}
BootstrapMethods_attribute::bootstrap_methods_t::~bootstrap_methods_t() { delete[] bootstrap_arguments; }

std::ifstream & operator >> (std::ifstream & f, BootstrapMethods_attribute & i) {
	f >> *((attribute_info *)&i);
	i.num_bootstrap_methods = read2(f);
	if (i.num_bootstrap_methods != 0)
		i.bootstrap_methods = new BootstrapMethods_attribute::bootstrap_methods_t[i.num_bootstrap_methods];
	for(int pos = 0; pos < i.num_bootstrap_methods; pos ++) {
		f >> i.bootstrap_methods[pos];
	}
	return f;
}
BootstrapMethods_attribute::~BootstrapMethods_attribute() { delete[] bootstrap_methods; }

std::ifstream & operator >> (std::ifstream & f, MethodParameters_attribute::parameters_t & i) {
	i.name_index = read2(f);
	i.access_flags = read2(f);
	return f;
}

std::ifstream & operator >> (std::ifstream & f, MethodParameters_attribute & i) {
	f >> *((attribute_info *)&i);
	i.parameters_count = read1(f);
	if (i.parameters_count != 0)
		i.parameters = new MethodParameters_attribute::parameters_t[i.parameters_count];
	for(int pos = 0; pos < i.parameters_count; pos ++) {
		f >> i.parameters[pos];
	}
	return f;
}
MethodParameters_attribute::~MethodParameters_attribute() { delete[] parameters; }

// aux function2
attribute_info* new_attribute(std::ifstream & f, cp_info **constant_pool) {		// new an attribute from ifstream using `peek`
	u2 attribute_name_index = peek2(f);
	int attribute_tag = peek_attribute(attribute_name_index, constant_pool);
	switch (attribute_tag) {
		case 0:	{
			ConstantValue_attribute* result = new ConstantValue_attribute;
			f >> *result;
			return result;
		}
		case 1:	{
			Code_attribute* result = new Code_attribute;
			result->fill(f, constant_pool);
			return result;
		}
		case 2:	{
			StackMapTable_attribute* result = new StackMapTable_attribute;
			f >> *result;
			return result;
		}
		case 3:	{
			Exceptions_attribute* result = new Exceptions_attribute;
			f >> *result;
			return result;
		}
		case 4:	{
			InnerClasses_attribute* result = new InnerClasses_attribute;
			f >> *result;
			return result;
		}
		case 5:	{
			EnclosingMethod_attribute* result = new EnclosingMethod_attribute;
			f >> *result;
			return result;
		}
		case 6:	{
			Synthetic_attribute* result = new Synthetic_attribute;
			f >> *result;
			return result;
		}
		case 7:	{
			Signature_attribute* result = new Signature_attribute;
			f >> *result;
			return result;
		}
		case 8:	{
			SourceFile_attribute* result = new SourceFile_attribute;
			f >> *result;
			return result;
		}
		case 9:	{
			SourceDebugExtension_attribute* result = new SourceDebugExtension_attribute;
			f >> *result;
			return result;
		}
		case 10:	{
			LineNumberTable_attribute* result = new LineNumberTable_attribute;
			f >> *result;
			return result;
		}
		case 11:	{
			LocalVariableTable_attribute* result = new LocalVariableTable_attribute;
			f >> *result;
			return result;
		}
		case 12:	{
			LocalVariableTypeTable_attribute* result = new LocalVariableTypeTable_attribute;
			f >> *result;
			return result;
		}
		case 13:	{
			Deprecated_attribute* result = new Deprecated_attribute;
			f >> *result;
			return result;
		}
		case 14:	{
			RuntimeVisibleAnnotations_attribute* result = new RuntimeVisibleAnnotations_attribute;
			f >> *result;
			return result;
		}
		case 15:	{
			RuntimeInvisibleAnnotations_attribute* result = new RuntimeInvisibleAnnotations_attribute;
			f >> *result;
			return result;
		}
		case 16:	{
			RuntimeVisibleParameterAnnotations_attribute* result = new RuntimeVisibleParameterAnnotations_attribute;
			f >> *result;
			return result;
		}
		case 17:	{
			RuntimeInvisibleParameterAnnotations_attribute* result = new RuntimeInvisibleParameterAnnotations_attribute;
			f >> *result;
			return result;
		}
		case 18:	{
			RuntimeVisibleTypeAnnotations_attribute* result = new RuntimeVisibleTypeAnnotations_attribute;
			f >> *result;
			return result;
		}
		case 19:	{
			RuntimeInvisibleTypeAnnotations_attribute* result = new RuntimeInvisibleTypeAnnotations_attribute;	// nashi
			f >> *result;
			return result;
		}
		case 20:	{
			AnnotationDefault_attribute* result = new AnnotationDefault_attribute;
			f >> *result;
			return result;
		}
		case 21:	{
			BootstrapMethods_attribute* result = new BootstrapMethods_attribute;
			f >> *result;
			return result;
		}
		case 22:	{
			MethodParameters_attribute* result = new MethodParameters_attribute;
			f >> *result;
			return result;
		}
		default:	{
			std::cerr << "can't go there! map has not this error tag " << attribute_tag << "!" << std::endl;
			assert(false);
			return nullptr;
		}
	}
}

/*===-----------  DEBUG Fields -----------===*/

void print_fields(field_info *bufs, int length, cp_info **constant_pool) {
	for(int i = 0; i < length; i ++) {
		std::stringstream ss;
		// parse access_flags
		int flags_num = 0;
		if ((bufs[i].access_flags & ACC_PUBLIC) == ACC_PUBLIC) {
			std::cout << "(DEBUG) public ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PUBLIC";
			flags_num ++;
		} else if ((bufs[i].access_flags & ACC_PRIVATE) == ACC_PRIVATE) {		// in fact, private and protected member shouldn't be output. haha.
			std::cout << "(DEBUG) private ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PRIVATE";
			flags_num ++;
		} else if ((bufs[i].access_flags & ACC_PROTECTED) == ACC_PROTECTED) {
			std::cout << "(DEBUG) protected ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PROTECTED";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_STATIC) == ACC_STATIC) {
			std::cout << "static ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_STATIC";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_FINAL) == ACC_FINAL) {
			std::cout << "final ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_FINAL";
			flags_num ++;
		} else if ((bufs[i].access_flags & ACC_VOLATILE) == ACC_VOLATILE) {
			std::cout << "volatile ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_VOLATILE";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_TRANSIENT) == ACC_TRANSIENT) {
			std::cout << "transient ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_TRANSIENT";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_SYNTHETIC) == ACC_SYNTHETIC) {
			std::cout << "synthetic ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_SYNTHETIC";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_ENUM) == ACC_ENUM) {
			std::cout << "enum ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_ENUM";
			flags_num ++;
		}
		// parse name_index
		assert (constant_pool[bufs[i].name_index-1]->tag == CONSTANT_Utf8);
		std::wcout << ((CONSTANT_Utf8_info *)constant_pool[bufs[i].name_index-1])->convert_to_Unicode() << L";\n";
		// parse descriptor_index
		assert (constant_pool[bufs[i].descriptor_index-1]->tag == CONSTANT_Utf8);
		std::wcout << "(DEBUG)   descriptor: " << ((CONSTANT_Utf8_info *)constant_pool[bufs[i].name_index-1])->convert_to_Unicode() << std::endl;
		// output flags
		std::cout << "(DEBUG)   flags: " << ss.str() << std::endl;
		// parse ConstantValue / Signature / RuntimeVisibleAnnotations / RuntimeInvisibleAnnotations
		for (int pos = 0; pos < bufs[i].attributes_count; pos ++) {
			int attribute_tag = peek_attribute(bufs[i].attributes[pos]->attribute_name_index, constant_pool);
			if (!(attribute_tag == 0 || attribute_tag == 6 || attribute_tag == 7 || attribute_tag == 13 || attribute_tag == 14 || attribute_tag == 15)) {
				// ignore (not 0, 6, 7, 13, 14, 15). in Java SE 8 Specification $ 4.5
				continue;
			}
			print_attributes(bufs[i].attributes[pos], constant_pool);		// I only implemented the '0' --> ConstantValue output. ?????????
		}
	}
}

/*===-----------  DEBUG Methods -----------===*/

int get_args_size (method_info *bufs, std::wstring & method_name, int i) {	// parse function name and get args_size.
	if (method_name.find(L"<init>") != -1 || method_name.find(L"<cinit>") != -1) {
		return 1;	
	}
	int left_pos = method_name.find_last_of('(');	assert(left_pos != -1);
	int right_pos = method_name.find_last_of(')');	assert(right_pos != -1);
	int args_size = 0;
	if ((bufs[i].access_flags & ACC_STATIC) != ACC_STATIC)	args_size ++;	// 'this' is the first arg.
	int comma_pos = left_pos;
	if (left_pos == right_pos - 1)	return args_size;		// 1. like `func()`
	else args_size ++;										// 2. else at lease like `func(int)`
															// 3. then, after finding a comma `,` args_size ++.
															//	  like `func(int, int, int)`
	while(1) {
		comma_pos = method_name.find_first_of(',', ++comma_pos);
		if (comma_pos != -1) {
			args_size ++;
		} else break;
	}
	return args_size;
};

void print_methods(method_info *bufs, int length, cp_info **constant_pool) {
	std::cout << "(DEBUG)" <<  std::endl;
	for(int i = 0; i < length; i ++) {
		std::stringstream ss;
		// parse access_flags
		int flags_num = 0;
		if ((bufs[i].access_flags & ACC_PUBLIC) == ACC_PUBLIC) {
			std::cout << "(DEBUG) public ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PUBLIC";
			flags_num ++;
		} else if ((bufs[i].access_flags & ACC_PRIVATE) == ACC_PRIVATE) {		// in fact, private and protected member shouldn't be output. haha.
			std::cout << "(DEBUG) private ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PRIVATE";
			flags_num ++;
		} else if ((bufs[i].access_flags & ACC_PROTECTED) == ACC_PROTECTED) {
			std::cout << "(DEBUG) protected ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_PROTECTED";
			flags_num ++;
		} else {
			std::cout << "(DEBUG) ";		// using for <clinit> : because <clinit> doesn't have a public/private/protected.
		}
		if ((bufs[i].access_flags & ACC_STATIC) == ACC_STATIC) {
			std::cout << "static ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_STATIC";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_FINAL) == ACC_FINAL) {
			std::cout << "final ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_FINAL";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_SYNCHRONIZED) == ACC_SYNCHRONIZED) {
			std::cout << "synchronized ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_SYNCHRONIZED";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_BRIDGE) == ACC_BRIDGE) {
			std::cout << "bridge ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_BRIDGE";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_VARARGS) == ACC_VARARGS) {
			std::cout << "va_args ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_VARARGS";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_NATIVE) == ACC_NATIVE) {
			std::cout << "native ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_NATIVE";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_ABSTRACT) == ACC_ABSTRACT) {
			std::cout << "abstract ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_ABSTRACT";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_STRICT) == ACC_STRICT) {
			std::cout << "strict ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_STRICT";
			flags_num ++;
		}
		if ((bufs[i].access_flags & ACC_SYNTHETIC) == ACC_SYNTHETIC) {
			std::cout << "synthetic ";
			if (flags_num != 0)		ss << ", ";
			ss << "ACC_SYNTHETIC";
			flags_num ++;
		}
		// parse name_index
		assert (constant_pool[bufs[i].name_index-1]->tag == CONSTANT_Utf8);
		std::wstring method_name = ((CONSTANT_Utf8_info *)constant_pool[bufs[i].name_index-1])->convert_to_Unicode();	// get function_name
		// first parse Exception_attribute to output name!! because there should be output `throws messages`! in fact this should be written in print_attributes() function...
		bool has_exception = false;
		for (int pos = 0; pos < bufs[i].attributes_count; pos ++) {
			int attribute_tag = peek_attribute(bufs[i].attributes[pos]->attribute_name_index, constant_pool);
			if (attribute_tag == 3) {
				if (has_exception == false) {
					has_exception = true;
					std::cout << " throws ";
				}
				auto *throws_ptr = (Exceptions_attribute *)bufs[i].attributes[pos];
				for (int k = 0; k < throws_ptr->number_of_exceptions; k ++) {
					assert (constant_pool[throws_ptr->exception_index_table[k]-1]->tag == CONSTANT_Class);	// throw a Exception class
					assert (constant_pool[((CONSTANT_CS_info *)constant_pool[throws_ptr->exception_index_table[k]-1])->index-1]->tag == CONSTANT_Utf8);
					std::wcout << ((CONSTANT_Utf8_info *)constant_pool[((CONSTANT_CS_info *)constant_pool[throws_ptr->exception_index_table[k]-1])->index-1])->convert_to_Unicode() << " ";
						// Exceptions_attribute -> CONSTANT_class_info -> CONSTANT_Utf8_info
				}
			}
		}
		std::wcout << method_name << L";\n";
		// parse descriptor_index
		assert (constant_pool[bufs[i].descriptor_index-1]->tag == CONSTANT_Utf8);
		std::wcout << "(DEBUG)   descriptor: " << ((CONSTANT_Utf8_info *)constant_pool[bufs[i].descriptor_index-1])->convert_to_Unicode() << std::endl;
		// output flags
		std::cout << "(DEBUG)   flags: " << ss.str() << std::endl;
		// parse ConstantValue / Signature / RuntimeVisibleAnnotations / RuntimeInvisibleAnnotations
		for (int pos = 0; pos < bufs[i].attributes_count; pos ++) {
			int attribute_tag = peek_attribute(bufs[i].attributes[pos]->attribute_name_index, constant_pool);
			if (!(attribute_tag == 1 || attribute_tag == 3 || attribute_tag == 6 ||
				  attribute_tag == 7 || attribute_tag == 13 || attribute_tag == 14 || 
				  attribute_tag == 15 || attribute_tag == 16 || attribute_tag == 17 || attribute_tag == 18)) {
				// ignore (not 1, 3, 6, 7, 13, 14, 15, 16, 17, 18). in Java SE 8 Specification $ 4.5
				continue;
			}
			print_attributes(bufs[i].attributes[pos], constant_pool);		// I only implemented the '0' --> ConstantValue output. ?????????
								// note: the args_size only used for Code_attribute.
		}
		std::cout << "(DEBUG)" << std::endl;
	}
}

/*===-----------  DEBUG Attributes -----------===*/

// bytecode map!
std::unordered_map<u1, std::pair<std::string, int>> bccode_map{	// pair<bccode_name, bccode_eat_how_many_arguments>
	{0x00, {"nop", 0}},
	{0x01, {"aconst_null", 0}},
	{0x02, {"iconst_m1", 0}},
	{0x03, {"iconst_0", 0}},
	{0x04, {"iconst_1", 0}},
	{0x05, {"iconst_2", 0}},
	{0x06, {"iconst_3", 0}},
	{0x07, {"iconst_4", 0}},
	{0x08, {"iconst_5", 0}},
	{0x09, {"lconst_0", 0}},
	{0x0a, {"lconst_1", 0}},
	{0x0b, {"fconst_0", 0}},
	{0x0c, {"fconst_1", 0}},
	{0x0d, {"fconst_2", 0}},
	{0x0e, {"dconst_0", 0}},
	{0x0f, {"dconst_1", 0}},
	{0x10, {"bipush", 1}},
	{0x11, {"sipush", 2}},
	{0x12, {"ldc", 1}},
	{0x13, {"ldc_w", 2}},
	{0x14, {"ldc2_w", 2}},
	{0x15, {"iload", 1}},
	{0x16, {"lload", 1}},
	{0x17, {"fload", 1}},
	{0x18, {"dload", 1}},
	{0x19, {"aload", 1}},
	{0x1a, {"iload_0", 0}},
	{0x1b, {"iload_1", 0}},
	{0x1c, {"iload_2", 0}},
	{0x1d, {"iload_3", 0}},
	{0x1e, {"lload_0", 0}},
	{0x1f, {"lload_1", 0}},
	{0x20, {"lload_2", 0}},
	{0x21, {"lload_3", 0}},
	{0x22, {"fload_0", 0}},
	{0x23, {"fload_1", 0}},
	{0x24, {"fload_2", 0}},
	{0x25, {"fload_3", 0}},
	{0x26, {"dload_0", 0}},
	{0x27, {"dload_1", 0}},
	{0x28, {"dload_2", 0}},
	{0x29, {"dload_3", 0}},
	{0x2a, {"aload_0", 0}},
	{0x2b, {"aload_1", 0}},
	{0x2c, {"aload_2", 0}},
	{0x2d, {"aload_3", 0}},
	{0x2e, {"iaload", 0}},
	{0x2f, {"laload", 0}},
	{0x30, {"faload", 0}},
	{0x31, {"daload", 0}},
	{0x32, {"aaload", 0}},
	{0x33, {"baload", 0}},
	{0x34, {"caload", 0}},
	{0x35, {"saload", 0}},
	{0x36, {"istore", 1}},
	{0x37, {"lstore", 1}},
	{0x38, {"fstore", 1}},
	{0x39, {"dstore", 1}},
	{0x3a, {"astore", 1}},
	{0x3b, {"istore_0", 0}},
	{0x3c, {"istore_1", 0}},
	{0x3d, {"istore_2", 0}},
	{0x3e, {"istore_3", 0}},
	{0x3f, {"lstore_0", 0}},
	{0x40, {"lstore_1", 0}},
	{0x41, {"lstore_2", 0}},
	{0x42, {"lstore_3", 0}},
	{0x43, {"fstore_0", 0}},
	{0x44, {"fstore_1", 0}},
	{0x45, {"fstore_2", 0}},
	{0x46, {"fstore_3", 0}},
	{0x47, {"dstore_0", 0}},
	{0x48, {"dstore_1", 0}},
	{0x49, {"dstore_2", 0}},
	{0x4a, {"dstore_3", 0}},
	{0x4b, {"astore_0", 1}},
	{0x4c, {"astore_1", 1}},
	{0x4d, {"astore_2", 1}},
	{0x4e, {"astore_3", 1}},
	{0x4f, {"iastore", 0}},
	{0x50, {"lastore", 0}},
	{0x51, {"fastore", 0}},
	{0x52, {"dastore", 0}},
	{0x53, {"aastore", 0}},
	{0x54, {"bastore", 0}},
	{0x55, {"castore", 0}},
	{0x56, {"sastore", 0}},
	{0x57, {"pop", 0}},
	{0x58, {"pop2", 0}},
	{0x59, {"dup", 0}},
	{0x5a, {"dup_x1", 0}},
	{0x5b, {"dup_x2", 0}},
	{0x5c, {"dup2", 0}},
	{0x5d, {"dup2_x1", 0}},
	{0x5e, {"dup2_x2", 0}},
	{0x5f, {"swap", 0}},
	{0x60, {"iadd", 0}},
	{0x61, {"ladd", 0}},
	{0x62, {"fadd", 0}},
	{0x63, {"dadd", 0}},
	{0x64, {"isub", 0}},
	{0x65, {"lsub", 0}},
	{0x66, {"fsub", 0}},
	{0x67, {"dsub", 0}},
	{0x68, {"imul", 0}},
	{0x69, {"lmul", 0}},
	{0x6a, {"fmul", 0}},
	{0x6b, {"dmul", 0}},
	{0x6c, {"idiv", 0}},
	{0x6d, {"ldiv", 0}},
	{0x6e, {"fdiv", 0}},
	{0x6f, {"ddiv", 0}},
	{0x70, {"irem", 0}},
	{0x71, {"lrem", 0}},
	{0x72, {"frem", 0}},
	{0x73, {"drem", 0}},
	{0x74, {"ineg", 0}},
	{0x75, {"lneg", 0}},
	{0x76, {"fneg", 0}},
	{0x77, {"dneg", 0}},
	{0x78, {"ishl", 0}},
	{0x79, {"lshl", 0}},
	{0x7a, {"ishr", 0}},
	{0x7b, {"lshr", 0}},
	{0x7c, {"iushr", 0}},
	{0x7d, {"lushr", 0}},
	{0x7e, {"iand", 0}},
	{0x7f, {"land", 0}},
	{0x80, {"ior", 0}},
	{0x81, {"lor", 0}},
	{0x82, {"ixor", 0}},
	{0x83, {"lxor", 0}},
	{0x84, {"iinc", 2}},
	{0x85, {"i2l", 0}},
	{0x86, {"i2f", 0}},
	{0x87, {"i2d", 0}},
	{0x88, {"l2i", 0}},
	{0x89, {"l2f", 0}},
	{0x8a, {"l2d", 0}},
	{0x8b, {"f2i", 0}},
	{0x8c, {"f2l", 0}},
	{0x8d, {"f2d", 0}},
	{0x8e, {"d2i", 0}},
	{0x8f, {"d2l", 0}},
	{0x90, {"d2f", 0}},
	{0x91, {"i2b", 0}},
	{0x92, {"i2c", 0}},
	{0x93, {"i2s", 0}},
	{0x94, {"lcmp", 0}},
	{0x95, {"fcmpl", 0}},
	{0x96, {"fcmpg", 0}},
	{0x97, {"dcmpl", 0}},
	{0x98, {"dcmpg", 0}},
	{0x99, {"ifeq", 2}},
	{0x9a, {"ifne", 2}},
	{0x9b, {"iflt", 2}},
	{0x9c, {"ifge", 2}},
	{0x9d, {"ifgt", 2}},
	{0x9e, {"ifle", 2}},
	{0x9f, {"if_icmpeq", 2}},
	{0xa0, {"if_icmpne", 2}},
	{0xa1, {"if_icmplt", 2}},
	{0xa2, {"if_icmpge", 2}},
	{0xa3, {"if_icmpgt", 2}},
	{0xa4, {"if_icmple", 2}},
	{0xa5, {"if_acmpeq", 2}},
	{0xa6, {"if_acmpne", 2}},
	{0xa7, {"goto", 2}},
	{0xa8, {"jsr", 2}},
	{0xa9, {"ret", 1}},
	{0xaa, {"tableswitch", -1}},	// va_args
	{0xab, {"lookupswitch", -1}},	// va_args
	{0xac, {"ireturn", 0}},
	{0xad, {"lreturn", 0}},
	{0xae, {"freturn", 0}},
	{0xaf, {"dreturn", 0}},
	{0xb0, {"areturn", 0}},
	{0xb1, {"return", 0}},
	{0xb2, {"getstatic", 2}},
	{0xb3, {"putstatic", 2}},
	{0xb4, {"getfield", 2}},
	{0xb5, {"putfield", 2}},
	{0xb6, {"invokevirtual", 2}},
	{0xb7, {"invokespecial", 2}},
	{0xb8, {"invokestatic", 2}},
	{0xb9, {"invokeinterface", 4}},	// 4.
	{0xba, {"invokedynamic", 4}},	// 4.
	{0xbb, {"new", 2}},
	{0xbc, {"newarray", 1}},
	{0xbd, {"anewarray", 2}},
	{0xbe, {"arraylength", 0}},
	{0xbf, {"athrow", 0}},
	{0xc0, {"checkcast", 2}},
	{0xc1, {"instanceof", 2}},
	{0xc2, {"monitorenter", 0}},
	{0xc3, {"monitorexit", 0}},
	{0xc4, {"wide", -2}},			// length 3 or 5. [extend local variable index]
	{0xc5, {"multianewarray", 3}},	// 3.
	{0xc6, {"ifnull", 2}},
	{0xc7, {"ifnonnull", 2}},
	{0xc8, {"goto_w", 4}},			// 4.
	{0xc9, {"jsr_w", 4}},			// 4.
	{0xca, {"breakpoint", 0}},
	{0xfe, {"impdep1", 0}},
	{0xff, {"impdep2", 0}},	
};

// aux pasing Runtime(In)visibleAnnotations.
// e.g. RuntimeInvisibleAnnotations:
// 			0: #12(#13=s#14,#15=@#10())
std::string recursive_parse_annotation (annotation *target) {
	std::stringstream total_str;
	total_str << "#" << target->type_index << "(";	// print key: #12
	
	// parse value
	std::function<std::string(element_value*)> parse_inner_element_value = [&parse_inner_element_value](element_value *inner_ev) -> std::string {
		std::stringstream ss;
		switch ((char)inner_ev->tag) {
			case 'B':case 'C':case 'D':case 'F':
			case 'I':case 'J':case 'S':case 'Z':
			case 's':{
				ss << "s#" << ((const_value_t *)inner_ev->value)->const_value_index;		// bug2: 指针强转不会有错误提示！！因此......这里原先写得是((const_value_t *)inner_ev)，但其实 inner_ev 内部的 inner_ev->value 才是真正应该被转换的......编译器不给报错！！
				break;
			}
			case 'e':{
				ss << "e#" << ((enum_const_value_t *)inner_ev->value)->const_name_index << "+"
							<< ((enum_const_value_t *)inner_ev->value)->type_name_index;
				break;
			}
			case 'c':{
				ss << "c#" << ((class_info_t *)inner_ev)->class_info_index;
				break;
			}
			case '@':{
				ss << "@" << recursive_parse_annotation((annotation *)inner_ev->value);	// recursive call outer function.
				break;
			}
			case '[':{
				ss << "[";
				int length = ((array_value_t *)inner_ev->value)->num_values;
				for (int pos = 0; pos < length; pos ++) {
							// bug 3 ！！调试时间最长的 bug！！在这里我不小心定义了和这个函数的参数一模一样的 inner_ev 变量！！在程序走到这里的时候，发生了如下错误！！重新定义一个和原先名字一模一样的变量 clang++ 竟然不报错吗 ???
							/**
								就好像:
								
								void haha(char **argv) 
								{
									{
										char **argv = argv;	// 不会有错误提示！唉（  g++ 和 clang++ 都没有错误提示...... 万一写错了咋办......看来只能自己小心了...
									}
								};

								*/
					// element_value *inner_ev = &((array_value_t *)inner_ev->value)->values[pos];	// error: Couldn't apply expression side effects : Couldn't dematerialize a result variable: couldn't read its memory
					element_value *inner_ev_2 = &((array_value_t *)inner_ev->value)->values[pos];
					ss << parse_inner_element_value(inner_ev_2);		// recursive !
					if (pos != length-1)	ss << ",";
				}
				ss << "]";
				break;
			}
			default:{
				std::cerr << "can't get here. in element_value." << std::endl;
				assert(false);
			}
		}
		return ss.str();
	};
	
	// bool is_first = false;
	for (int j = 0; j < target->num_element_value_pairs; j ++) {
		total_str << "#" << target->element_value_pairs[j].element_name_index << "=";	// inner value's key: #13
		element_value *inner_ev = &target->element_value_pairs[j].value;
		total_str << parse_inner_element_value(inner_ev);
		if (j != target->num_element_value_pairs-1)	total_str << ",";
	}

	total_str << ")";
	return total_str.str();
};

// 注意：支持中文变量名(应该全改成 wstring 才行)??
std::wstring get_classname_from_constpool(int index, cp_info **constant_pool) {
	assert(constant_pool[index-1]->tag == CONSTANT_Class);	// assert index is a Class type.	// then get classname: Unicode
	return ((CONSTANT_Utf8_info *)constant_pool[((CONSTANT_CS_info *)constant_pool[index-1])->index-1])->convert_to_Unicode();
}

std::wstring print_verfication(StackMapTable_attribute::verification_type_info** ptr, int length, cp_info **constant_pool, int local_or_stack = 0) {
	std::wstringstream ss;
	if (local_or_stack == 0)
		ss << "locals = [";
	else 
		ss << "stack = [";
	for (int i = 0; i < length; i ++) {
		int tag = ptr[i]->tag;
		switch(tag) {
			case ITEM_Top:{
				ss << "Top";
				break;
			}
			case ITEM_Integer:{
				ss << "Integer";
				break;
			}
			case ITEM_Float:{
				ss << "Float";
				break;
			}
			case ITEM_Double:{
				ss << "Double";
				break;
			}
			case ITEM_Long:{
				ss << "Long";
				break;
			}
			case ITEM_Null:{
				ss << "Null";
				break;
			}
			case ITEM_UninitializedThis:{
				ss << "UninitializedThis";
				break;
			}
			case ITEM_Object:{
				// ss << ((StackMapTable_attribute::Object_variable_info *)ptr[i])->cpool_index << "+" << "Object";
				ss << get_classname_from_constpool(((StackMapTable_attribute::Object_variable_info *)ptr[i])->cpool_index, constant_pool);
				break;
			}
			case ITEM_Uninitialized:{
				ss << "offset: " << ((StackMapTable_attribute::Uninitialized_variable_info *)ptr[i])->offset << "+" << " Uninitialized";
				break;
			}
			default:{
				std::cerr << "can't arrive here! in print_verfication()." << std::endl;
				assert(false);
			}
		}
		if (i != length-1)	ss << ", ";
	}
	ss << "]";
	return ss.str();
}

void print_attributes(attribute_info *ptr, cp_info **constant_pool) {
	int attribute_tag = peek_attribute(ptr->attribute_name_index, constant_pool);
	switch (attribute_tag) {
		case 0: {		// ConstantValue
			std::cout << "(DEBUG)   ConstantValue: ";
			u2 index = ((ConstantValue_attribute *)ptr)->constantvalue_index;
			switch (constant_pool[index-1]->tag) {
				case CONSTANT_Long:{
					std::cout << "long " << ((CONSTANT_Long_info *)constant_pool[index-1])->get_value() << "l" << std::endl;
					break;
				}
				case CONSTANT_Float:{
					float result = ((CONSTANT_Float_info *)constant_pool[index-1])->get_value();
					std::cout << "float ";
					if (result == FLOAT_INFINITY)
						std::cout << "Infinityf" << std::endl;
					else if (result == FLOAT_NEGATIVE_INFINITY)
						std::cout << "-Infinityf" << std::endl;
					else if (result == FLOAT_NAN)
						std::cout << "NaNf" << std::endl;
					else 
						std::cout << result << "f" << std::endl;
					break;
				}
				case CONSTANT_Integer:{
					std::cout << "int " << ((CONSTANT_Integer_info *)constant_pool[index-1])->get_value() << std::endl;
					break;
				}
				case CONSTANT_Double:{
					double result = ((CONSTANT_Double_info *)constant_pool[index-1])->get_value();
					std::cout << "double ";
					if (result == DOUBLE_INFINITY)
						std::cout << "Infinityd" << std::endl;
					else if (result == DOUBLE_NEGATIVE_INFINITY)
						std::cout << "-Infinityd" << std::endl;
					else if (result == DOUBLE_NAN)
						std::cout << "NaNd" << std::endl;
					else 
						std::cout << result << "d" << std::endl;
					break;
				}
				case CONSTANT_String:{
					// search for two times.
					u2 string_index = ((CONSTANT_CS_info *)constant_pool[index-1])->index;
					assert (constant_pool[string_index-1]->tag == CONSTANT_Utf8);
					std::cout << "String ";
					std::wcout.imbue(std::locale(""));
					std::wcout << ((CONSTANT_Utf8_info *)constant_pool[string_index-1])->convert_to_Unicode() << std::endl;
					break;
				}
				default:{
					std::cerr << "can't get here! in print_attributes()." << std::endl;
					assert (false);
				}
			}
			break;
		}
		case 1:{	// Code
			std::cout << "(DEBUG)   Code: " << std::endl;
			auto *code_ptr = (Code_attribute *)ptr;
			std::cout << "(DEBUG)     stack=" << code_ptr->max_stack << ", locals=" << code_ptr->max_locals /*<< ", args_size=" << args_size*/ << std::endl;	// output arg_size need parse descriptor. not important.
			// output bccode
			for (int bc_num = 0; bc_num < code_ptr->code_length; bc_num ++) {
				std::cout << "(DEBUG)     ";
				printf("%3d: %-15s", bc_num, bccode_map[code_ptr->code[bc_num]].first.c_str()); 		// other message is to big, I dont want to save them.
				if (bccode_map[bc_num].second >= 0) {	
					bc_num += bccode_map[code_ptr->code[bc_num]].second;
				} else {		// 变长参数 以及 扩展局部变量索引 分别被定义为 -1 -2。 这里需要改进 !!!!!
					
				}
				std::cout << std::endl;		// ....... 还有其他的参数没有输出......????
			}
			// output code exception table
			if (code_ptr->exception_table_length != 0) {
				std::cout << "(DEBUG)     " << "Exception Table:" << std::endl;
				printf("(DEBUG)%8sfrom%6sto%2starget%4stype\n", "", "", "", "");
			}
			for (int pos = 0; pos < code_ptr->exception_table_length; pos ++) {
				auto *table = &code_ptr->exception_table[pos];
				printf("(DEBUG)%8s%4d%4s%4d%4s%4d%4s%4d\n", "", table->start_pc, "", table->end_pc, "", table->handler_pc, "", table->catch_type);
			}
			// output LineNumberTable、StackMapTable etc. (attributes inner Code Area)
			for (int pos = 0; pos < code_ptr->attributes_count; pos ++) {
				print_attributes(code_ptr->attributes[pos], constant_pool);	// print other attributes.
			}
			break;
		}
		case 2:{	// StackMapTable (Inner Code Attribute)
			auto *smt_ptr = (StackMapTable_attribute *)ptr;
			std::cout << "(DEBUG)     StackMapTable: number_of_entries = " << smt_ptr->number_of_entries << std::endl;
			for (int pos = 0; pos < smt_ptr->number_of_entries; pos ++) {
				auto *entry = smt_ptr->entries[pos];
				if (entry->frame_type >= 0 && entry->frame_type <= 63) {
					auto *frame = (StackMapTable_attribute::same_frame *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* same */" << std::endl;
				} else if (entry->frame_type >= 64 && entry->frame_type <= 127) {
					auto *frame = (StackMapTable_attribute::same_locals_1_stack_item_frame *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* stack_item */" << std::endl;
					std::wcout << "(DEBUG)         " << print_verfication(&frame->stack[0], 1, constant_pool) << std::endl;
				} else if (entry->frame_type == 247) {
					auto *frame = (StackMapTable_attribute::same_locals_1_stack_item_frame_extended *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* stack_item_extended */" << std::endl;
					std::cout << "(DEBUG)         offset_delta = " << frame->offset_delta << std::endl;
					std::wcout << "(DEBUG)         " << print_verfication(&frame->stack[0], 1, constant_pool) << std::endl;
				} else if (entry->frame_type >= 248 && entry->frame_type <= 250) {
					auto *frame = (StackMapTable_attribute::chop_frame *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* chop */" << std::endl;
					std::cout << "(DEBUG)         offset_delta = " << frame->offset_delta << std::endl;
				} else if (entry->frame_type == 251) {
					auto *frame = (StackMapTable_attribute::same_frame_extended *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* same_extended */" << std::endl;
					std::cout << "(DEBUG)         offset_delta = " << frame->offset_delta << std::endl;
				} else if (entry->frame_type >= 252 && entry->frame_type <= 254) {
					auto *frame = (StackMapTable_attribute::append_frame *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* append */" << std::endl;
					std::cout << "(DEBUG)         offset_delta = " << frame->offset_delta << std::endl;
					std::wcout << "(DEBUG)         " << print_verfication(frame->locals, frame->frame_type-251, constant_pool) << std::endl;
				} else if (entry->frame_type == 255) {
					auto *frame = (StackMapTable_attribute::full_frame *)entry;
					std::cout << "(DEBUG)       frame_type = " << dec << (int)frame->frame_type << "  /* full */" << std::endl;
					std::cout << "(DEBUG)         offset_delta = " << frame->offset_delta << std::endl;
					std::wcout << "(DEBUG)         " << print_verfication(frame->locals, frame->number_of_locals, constant_pool, 0) << std::endl;
					std::wcout << "(DEBUG)         " << print_verfication(frame->stack, frame->number_of_stack_items, constant_pool, 1) << std::endl;
				} else {
					std::cerr << "can't get here! in peek_stackmaptable_frame()" << std::endl;
					assert(false);
				}
			}
			break;
		}
		case 3:{	// Exceptions
			std::cout << "(DEBUG)   Exceptions: " << std::endl;
			auto *throws_ptr = (Exceptions_attribute *)ptr;
			std::cout << "(DEBUG)     throws ";
			for (int k = 0; k < throws_ptr->number_of_exceptions; k ++) {		// same as print_method().
				assert (constant_pool[throws_ptr->exception_index_table[k]-1]->tag == CONSTANT_Class);	// throw a Exception class
				assert (constant_pool[((CONSTANT_CS_info *)constant_pool[throws_ptr->exception_index_table[k]-1])->index-1]->tag == CONSTANT_Utf8);
				std::wcout << ((CONSTANT_Utf8_info *)constant_pool[((CONSTANT_CS_info *)constant_pool[throws_ptr->exception_index_table[k]-1])->index-1])->convert_to_Unicode() << " ";
					// Exceptions_attribute -> CONSTANT_class_info -> CONSTANT_Utf8_info
			}
			std::cout << std::endl << "(DEBUG)" << std::endl;
			break;
		}
		case 6:{	// Synthetic
			std::cout << "(DEBUG)   Synthetic: (length) " << ptr->attribute_length << std::endl;
			break;
		}
		case 7:{	// Signature
			auto *signature_ptr = (Signature_attribute *)ptr;
			assert (constant_pool[signature_ptr->signature_index-1]->tag == CONSTANT_Utf8);
			std::wcout << "(DEBUG)   Signature: " << ((CONSTANT_Utf8_info *)constant_pool[signature_ptr->signature_index-1])->convert_to_Unicode() << std::endl;
			break;
		}
		case 10:{	// LineNumberTable (Inside the Code Attribution)
			std::cout << "(DEBUG)     LineNumberTable:" << std::endl;
			auto *line_table = (LineNumberTable_attribute *)ptr;
			for(int pos = 0; pos < line_table->line_number_table_length; pos ++) {
				auto *table = &line_table->line_number_table[pos];
				printf("(DEBUG)%7sline: %4d, start_pc: %-4d\n", "", table->line_number, table->start_pc);
			}
			break;
		}
		case 13:{	// Deprecated
			std::wcout << "(DEBUG)   Deprecated: true" << std::endl;
			break;
		}
		case 14:
		case 15:{	// Runtime(In)VisibleAnnotations
			if (attribute_tag == 14)
				std::cout << "(DEBUG)   RuntimeVisibleAnnotations:" << std::endl;
			else
				std::cout << "(DEBUG)   RuntimeInisibleAnnotations:" << std::endl;
			auto *annotations_ptr = (RuntimeVisibleAnnotations_attribute *)ptr;
			for (int i = 0; i < annotations_ptr->parameter_annotations.num_annotations; i ++) {
				annotation *target = &annotations_ptr->parameter_annotations.annotations[i];
				std::cout << "(DEBUG)     " << i << ": ";
				std::cout << recursive_parse_annotation(target) << std::endl;
			}
			break;
		}
		case 16:
		case 17:{
			if (attribute_tag == 16)
				std::cout << "(DEBUG)   RuntimeVisibleParameterAnnotations:" << std::endl;
			else
				std::cout << "(DEBUG)   RuntimeInisibleParameterAnnotations:" << std::endl;
			auto *annotations_ptr = (RuntimeVisibleParameterAnnotations_attribute *)ptr;
			for (int i = 0; i < annotations_ptr->num_parameters; i ++) {
				for (int j = 0; j < annotations_ptr->parameter_annotations[i].num_annotations; j ++) {
					annotation *target = &annotations_ptr->parameter_annotations[i].annotations[j];
					std::cout << "(DEBUG)     " << j << ": ";
					std::cout << recursive_parse_annotation(target) << std::endl;
				}
			}
			break;
		}
	}
}

/*===----------- .class ----------------===*/

std::ifstream & operator >> (std::ifstream & f, ClassFile & cf) {
	
	cf.parse_header(f);
	cf.parse_constant_pool(f);
	cf.parse_class_msgs(f);
	cf.parse_interfaces(f);
	cf.parse_fields(f);
	cf.parse_methods(f);

	return f;
}
		
ClassFile::~ClassFile() {
	if (constant_pool != nullptr) {
		for(int i = 0; i < constant_pool_count-1; i ++) {
			delete constant_pool[i];
			if(constant_pool[i]->tag == CONSTANT_Long || constant_pool[i]->tag == CONSTANT_Double)	i ++;
		}
	}
	delete[] constant_pool;
	delete[] interfaces;
	delete[] fields;
		
	// todo: delete others memory !!!!!!!!!!!
}

void ClassFile::parse_header(std::ifstream & f) {
	// for header
	f.read((char *)&magic, sizeof(magic));
	magic = htonl(magic);
	if(magic != MAGIC_NUMBER) {
		cout << "can't recognize this file!" << endl;
		exit(1);
	}
	f.read((char *)&minor_version, sizeof(minor_version));
	f.read((char *)&major_version, sizeof(major_version));
	f.read((char *)&constant_pool_count, sizeof(constant_pool_count));
	minor_version = htons(minor_version);
	major_version = htons(major_version);
	constant_pool_count = htons(constant_pool_count);
#ifdef DEBUG
	cout << hex  << "(DEBUG) " << magic << " " << dec << minor_version << " " << major_version << " " << constant_pool_count << endl;
#endif
}

void ClassFile::parse_constant_pool(std::ifstream & f) {
	// for constant pool
	if (constant_pool_count != 1)
		constant_pool = new cp_info*[constant_pool_count - 1];
	for(int i = 0; i < constant_pool_count-1; i ++) {
		u1 tag = peek1(f);
//		cout << i+1 << " " << (int)tag << endl;
		switch (tag) {
			case CONSTANT_Class:
			case CONSTANT_String:
				constant_pool[i] = new CONSTANT_CS_info;
				f >> *(CONSTANT_CS_info*)constant_pool[i];
				break;
			case CONSTANT_Fieldref:
			case CONSTANT_Methodref:
			case CONSTANT_InterfaceMethodref:
				constant_pool[i] = new CONSTANT_FMI_info;
				f >> *(CONSTANT_FMI_info*)constant_pool[i];
				break;
			case CONSTANT_Integer:
				constant_pool[i] = new CONSTANT_Integer_info;
				f >> *(CONSTANT_Integer_info*)constant_pool[i];
				break;
			case CONSTANT_Float:
				constant_pool[i] = new CONSTANT_Float_info;
				f >> *(CONSTANT_Float_info*)constant_pool[i];
				break;
			case CONSTANT_Long:
				constant_pool[i] = new CONSTANT_Long_info;
				f >> *(CONSTANT_Long_info*)constant_pool[i];
				i ++;	// long and double should take 2 positions of constant pool.
				break;
			case CONSTANT_Double:
				constant_pool[i] = new CONSTANT_Double_info;
				f >> *(CONSTANT_Double_info*)constant_pool[i];
				i ++;	// long and double should take 2 positions of constant pool.
				break;
			case CONSTANT_NameAndType:
				constant_pool[i] = new CONSTANT_NameAndType_info;
				f >> *(CONSTANT_NameAndType_info*)constant_pool[i];
				break;
			case CONSTANT_Utf8:
				constant_pool[i] = new CONSTANT_Utf8_info;
				f >> *(CONSTANT_Utf8_info*)constant_pool[i];
				break;
			case CONSTANT_MethodHandle:
				constant_pool[i] = new CONSTANT_MethodHandle_info;
				f >> *(CONSTANT_MethodHandle_info*)constant_pool[i];
				break;
			case CONSTANT_MethodType:
				constant_pool[i] = new CONSTANT_MethodType_info;
				f >> *(CONSTANT_MethodType_info*)constant_pool[i];
				break;
			case CONSTANT_InvokeDynamic:
				constant_pool[i] = new CONSTANT_InvokeDynamic_info;
				f >> *(CONSTANT_InvokeDynamic_info*)constant_pool[i];
				break;
			default:
				std::cout << "can't get here, CONSTANT pool." << std::endl;
				assert(false);
		}
	}
#ifdef DEBUG
	std::cout << "(DEBUG) constpool_size: " << constant_pool_count << std::endl;
	print_constant_pool(constant_pool, constant_pool_count-1);
#endif
}

void ClassFile::parse_class_msgs(std::ifstream & f) {
	access_flags = read2(f);
	this_class = read2(f);
	super_class = read2(f);
#ifdef DEBUG
	std::cout << "(DEBUG) access_flags: " << access_flags << "  this_class: " << this_class << "  super_class:" << super_class << endl;
#endif
}

void ClassFile::parse_interfaces(std::ifstream & f) {
	interfaces_count = read2(f);
	if (interfaces_count != 0)	interfaces = new u2[interfaces_count];
	for(int i = 0; i < interfaces_count; i ++) {
		interfaces[i] = read2(f);
	}
#ifdef DEBUG
	if (interfaces_count != 0) {
		std::cout << "(DEBUG) Interfaces: ";
		for(int i = 0; i < interfaces_count; i ++) {
			std::cout << interfaces[i] << " ";
		}
		std::cout << std::endl;
	} else {
		std::cout << "(DEBUG) no Interfaces" << std::endl;
	}
#endif
}

void ClassFile::parse_fields(std::ifstream & f) {
	fields_count = read2(f);
	if (fields_count != 0)
		fields = new field_info[fields_count];
	for(int pos = 0; pos < fields_count; pos ++) {
		fields[pos].fill(f, constant_pool);
	}
#ifdef DEBUG
	if (fields_count != 0) {
		std::cout << "(DEBUG) field_info_size: " << fields_count << std::endl;
		print_fields(fields, fields_count, constant_pool);
	} else {
		std::cout << "(DEBUG) no fields variables" << std::endl;
	}
#endif
}

void ClassFile::parse_methods(std::ifstream & f) {
	methods_count = read2(f);
	if (methods_count != 0)
		methods = new method_info[methods_count];
	for(int pos = 0; pos < methods_count; pos ++) {
		methods[pos].fill(f, constant_pool);
	}
#ifdef DEBUG
	if (methods_count != 0) {
		std::cout << "(DEBUG) method_number: " << methods_count << std::endl;
		print_methods(methods, methods_count, constant_pool);
	} else {
		std::cout << "(DEBUG) no method functions" << std::endl;
	}
#endif
}

int main(int argc, char *argv[]) 
{
	std::wcout.imbue(std::locale(""));
	std::ifstream f("Annotations.class", std::ios::binary);
	if(!f.is_open()) {
		cout << "wrong!" << endl;
		return -1;
 	}
	
	ClassFile cf;
	// magic number
	f >> cf;
	
}
#include "runtime/method.hpp"
#include "runtime/klass.hpp"
#include "utils/utils.hpp"
#include "runtime/oop.hpp"
#include "runtime/bytecodeEngine.hpp"
#include "native/java_lang_Class.hpp"
#include "classloader.hpp"
#include "utils/synchronize_wcout.hpp"

Method::Method(shared_ptr<InstanceKlass> klass, method_info & mi, cp_info **constant_pool) : klass(klass) {
	assert(constant_pool[mi.name_index-1]->tag == CONSTANT_Utf8);
	name = ((CONSTANT_Utf8_info *)constant_pool[mi.name_index-1])->convert_to_Unicode();
	assert(constant_pool[mi.descriptor_index-1]->tag == CONSTANT_Utf8);
	descriptor = ((CONSTANT_Utf8_info *)constant_pool[mi.descriptor_index-1])->convert_to_Unicode();
	access_flags = mi.access_flags;

	// move!!! important!!!
	this->attributes = mi.attributes;
	mi.attributes = nullptr;

	// set to 0!!! important!!!
	this->attributes_count = mi.attributes_count;
	mi.attributes_count = 0;

	for(int i = 0; i < this->attributes_count; i ++) {
		int attribute_tag = peek_attribute(this->attributes[i]->attribute_name_index, constant_pool);
		switch (attribute_tag) {	// must be 1, 3, 6, 7, 13, 14, 15, 16, 17, 18, 19, 20, 22 for Method.
								// must be 2, 10, 11, 12, 18, 19 for Code attribute.
			case 1:{	// Code
				code = (Code_attribute *)this->attributes[i];
				for (int pos = 0; pos < code->attributes_count; pos ++) {
					int code_attribute_tag = peek_attribute(code->attributes[pos]->attribute_name_index, constant_pool);
					switch (code_attribute_tag) {
						case 18:{		// RuntimeVisibleTypeAnnotation
							auto enter_ptr = ((RuntimeVisibleTypeAnnotations_attribute *)code->attributes[pos]);
							this->Code_num_RuntimeVisibleTypeAnnotations = enter_ptr->num_annotations;
							this->Code_rvta = (TypeAnnotation *)malloc(sizeof(TypeAnnotation) * this->Code_num_RuntimeVisibleTypeAnnotations);
							for (int pos = 0; pos < this->Code_num_RuntimeVisibleTypeAnnotations; pos ++) {
								constructor(&this->Code_rvta[pos], constant_pool, enter_ptr->annotations[pos]);
							}
							break;
						}
						case 10:{		// LineNumberTable: 用于 printStackTrace。
							this->lnt = ((LineNumberTable_attribute *)code->attributes[pos]);
							code->attributes[pos] = nullptr;		// move 语义！ **IMPORTANT**
							break;
						}
						case 2:
						case 11:
						case 12:
						case 19:{
							break;
						}
						default:{
							std::wcerr << "Annotations are TODO! attribute_tag == " << code_attribute_tag << " in Method: [" << name << "]" << std::endl;
							assert(false);
						}
					}
				}
				break;
			}
			case 3:{	// Exception
				exceptions = (Exceptions_attribute *)this->attributes[i];
				break;
			}
			case 7:{	// Signature
				signature_index = ((Signature_attribute *)this->attributes[i])->signature_index;
				break;
			}
			case 14:{	// RuntimeVisibleAnnotation
				auto enter = ((RuntimeVisibleAnnotations_attribute *)this->attributes[i])->parameter_annotations;
				this->rva = (Parameter_annotations_t *)malloc(sizeof(Parameter_annotations_t));
				constructor(this->rva, constant_pool, enter);
				break;
			}
			case 16:{	// RuntimeVisibleParameterAnnotation
				auto enter_ptr = ((RuntimeVisibleParameterAnnotations_attribute *)this->attributes[i]);
				this->num_RuntimeVisibleParameterAnnotation = enter_ptr->num_parameters;
				this->rvpa = (Parameter_annotations_t *)malloc(sizeof(Parameter_annotations_t) * this->num_RuntimeVisibleParameterAnnotation);
				for (int pos = 0; pos < this->num_RuntimeVisibleParameterAnnotation; pos ++) {
					constructor(&this->rvpa[pos], constant_pool, enter_ptr->parameter_annotations[pos]);
				}
				this->_rvpa = enter_ptr->stub;
				break;
			}
			case 18:{		// RuntimeVisibleTypeAnnotation
				auto enter_ptr = ((RuntimeVisibleTypeAnnotations_attribute *)this->attributes[i]);
				this->num_RuntimeVisibleTypeAnnotations = enter_ptr->num_annotations;
				this->rvta = (TypeAnnotation *)malloc(sizeof(TypeAnnotation) * this->num_RuntimeVisibleTypeAnnotations);
				for (int pos = 0; pos < this->num_RuntimeVisibleTypeAnnotations; pos ++) {
					constructor(&this->rvta[pos], constant_pool, enter_ptr->annotations[pos]);
				}
				break;
			}
			case 20:{		// Annotation Default
				auto element_value = ((AnnotationDefault_attribute *)this->attributes[i])->default_value;
				this->ad = (Element_value *)malloc(sizeof(Element_value));
				constructor(this->ad, constant_pool, element_value);
				this->_ad = ((AnnotationDefault_attribute *)this->attributes[i])->stub;
				break;
			}
			case 6:
			case 13:
			case 15:
			case 17:
			case 19:
			case 22:
			{	// do nothing
				break;
			}
			default:{
				std::cerr << "attribute_tag == " << attribute_tag << std::endl;
				assert(false);
			}
		}

	}
}


wstring Method::parse_signature()
{
	if (signature_index == 0) return L"";
	auto _pair = (*klass->get_rtpool())[signature_index];
	assert(_pair.first == CONSTANT_Utf8);
	wstring signature = boost::any_cast<wstring>(_pair.second);
	assert(signature != L"");	// 别和我设置为空而返回的 L"" 重了.....
	return signature;
}

int Method::get_java_source_lineno(int pc_no)
{
	assert(pc_no >= 0);
	if (pc_no == 0) {
		// native method.
		return 0;
	}
	// if we need to call this, it must be printStackTrace.
	assert(this->lnt != nullptr);
	for (int i = 0; i < this->lnt->line_number_table_length; i ++) {
		if (this->lnt->line_number_table[i].start_pc <= pc_no) {
			return this->lnt->line_number_table[i].line_number;
		}
	}
	assert(false);
}

vector<MirrorOop *> Method::if_didnt_parse_exceptions_then_parse()
{
	if (!parsed) {
		parsed = true;
		if (exceptions != nullptr)
			for (int i = 0; i < exceptions->number_of_exceptions; i ++) {
				auto rt_pool = this->klass->get_rtpool();
				assert((*rt_pool)[exceptions->exception_index_table[i]-1].first == CONSTANT_Class);
				auto excp_klass = boost::any_cast<shared_ptr<Klass>>((*rt_pool)[exceptions->exception_index_table[i]-1].second);
				exceptions_tb[excp_klass->get_name()] = excp_klass;
			}
	}

	vector<MirrorOop *> v;
	for (auto iter : exceptions_tb) {
		v.push_back(iter.second->get_mirror());
	}
	return v;
}

vector<MirrorOop *> Method::parse_argument_list()
{
	if (real_descriptor == L"")
		return parse_argument_list(this->descriptor);
	else
		return parse_argument_list(this->real_descriptor);		// patch for MethodHandle.invoke***(Object...)
}

MirrorOop *Method::parse_return_type()
{
	wstring return_type = this->return_type();
	return parse_return_type(return_type);
}

vector<MirrorOop *> Method::parse_argument_list(const wstring & descriptor)
{
	vector<MirrorOop *> v;
	vector<wstring> args = BytecodeEngine::parse_arg_list(descriptor);
	for (int i = 0; i < args.size(); i ++) {
		if (args[i].size() == 1) {		// primitive type
			switch (args[i][0]) {
				case L'Z':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"Z"));
					break;
				}
				case L'B':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"B"));
					break;
				}
				case L'S':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"S"));
					break;
				}
				case L'C':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"C"));
					break;
				}
				case L'I':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"I"));
					break;
				}
				case L'F':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"F"));
					break;
				}
				case L'J':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"J"));
					break;
				}
				case L'D':{
					v.push_back(java_lang_class::get_basic_type_mirror(L"D"));
					break;
				}
				default:{
					assert(false);
				}
			}
		} else if (args[i][0] == L'L') {	// InstanceOop type
//			ClassLoader *loader = this->klass->get_classloader();		// bug report: 不要使用此 Method 的 classLoader ！！因为完全有可能是 invoke 方法(BootStrap 加载 java/lang/invoke...)，invoke 了一个 MyclassLoader 加载的类......
//			shared_ptr<Klass> klass;
//			if (loader == nullptr) {
//				klass = BootStrapClassLoader::get_bootstrap().loadClass(args[i].substr(1, args[i].size() - 2));
//			} else {
//				klass = loader->loadClass(args[i].substr(1, args[i].size() - 2));
//			}
			shared_ptr<Klass> klass = MyClassLoader::get_loader().loadClass(args[i].substr(1, args[i].size() - 2));
			assert(klass != nullptr);
			v.push_back(klass->get_mirror());
		} else {		// ArrayType
			assert(args[i][0] == L'[');
//			ClassLoader *loader = this->klass->get_classloader();
//			shared_ptr<Klass> klass;
//			if (loader == nullptr) {
//				klass = BootStrapClassLoader::get_bootstrap().loadClass(args[i]);
//			} else {
//				klass = loader->loadClass(args[i]);
//			}
			shared_ptr<Klass> klass = MyClassLoader::get_loader().loadClass(args[i]);
			assert(klass != nullptr);
			v.push_back(klass->get_mirror());
		}
	}
#ifdef DEBUG
	sync_wcout{} << "===---------------- arg list (Runtime of " << descriptor << ") -----------------===" << std::endl;
	for (int i = 0; i < v.size(); i ++) {
		sync_wcout{} << v[i]->get_klass()->get_name() << std::endl;
	}
	sync_wcout{} << "===--------------------------------------------------------===" << std::endl;
#endif
	return v;
}

MirrorOop *Method::parse_return_type(const wstring & return_type)
{
	if (return_type.size() == 1) {		// primitive type
		switch (return_type[0]) {
			case L'Z':{
				return java_lang_class::get_basic_type_mirror(L"Z");
			}
			case L'B':{
				return java_lang_class::get_basic_type_mirror(L"B");
			}
			case L'S':{
				return java_lang_class::get_basic_type_mirror(L"S");
			}
			case L'C':{
				return java_lang_class::get_basic_type_mirror(L"C");
			}
			case L'I':{
				return java_lang_class::get_basic_type_mirror(L"I");
			}
			case L'F':{
				return java_lang_class::get_basic_type_mirror(L"F");
			}
			case L'J':{
				return java_lang_class::get_basic_type_mirror(L"J");
			}
			case L'D':{
				return java_lang_class::get_basic_type_mirror(L"D");
			}
			case L'V':{			// **IMPORTANT** the return type `V` is the java/lang/Void!!!
				return java_lang_class::get_basic_type_mirror(L"V");
//				return BootStrapClassLoader::get_bootstrap().loadClass(L"java/lang/Void")->get_mirror();		// bug fix.
			}
			default:{
				assert(false);
			}
		}
	} else if (return_type[0] == L'L') {	// InstanceOop type
//		ClassLoader *loader = this->klass->get_classloader();
//		shared_ptr<Klass> klass;
//		if (loader == nullptr) {
//			klass = BootStrapClassLoader::get_bootstrap().loadClass(return_type.substr(1, return_type.size() - 2));
//		} else {
//			klass = loader->loadClass(return_type.substr(1, return_type.size() - 2));
//		}
		shared_ptr<Klass> klass = MyClassLoader::get_loader().loadClass(return_type.substr(1, return_type.size() - 2));
		assert(klass != nullptr);
		return klass->get_mirror();
	} else {		// ArrayType
		assert(return_type[0] == L'[');
//		ClassLoader *loader = this->klass->get_classloader();
//		shared_ptr<Klass> klass;
//		if (loader == nullptr) {
//			klass = BootStrapClassLoader::get_bootstrap().loadClass(return_type);
//		} else {
//			klass = loader->loadClass(return_type);
//		}
		shared_ptr<Klass> klass = MyClassLoader::get_loader().loadClass(return_type);
		assert(klass != nullptr);
		return klass->get_mirror();
	}
}

int Method::where_is_catch(int cur_pc, shared_ptr<InstanceKlass> cur_excp)
{
#ifdef DEBUG
	sync_wcout{} << "===-------------- [Begin Finding Catch/Finally Block...] -----------------===" << std::endl;
#endif

	// 可以改进：这里其实可以考虑设置一个计数器什么的。因为之前被 catch 的肯定不会被第二次 catch。catch 这东西，对于同一个对象而言，是一次性的。
	auto rt_pool = this->get_klass()->get_rtpool();
	for (int i = 0; i < this->code->exception_table_length; i ++) {
		auto excp_tbl = this->code->exception_table[i];
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) cur_pc is: [" << cur_pc << "], and it compare with: [" << excp_tbl.start_pc << "~" << excp_tbl.end_pc << "], " << std::endl;
#endif
		if (cur_pc >= excp_tbl.start_pc && cur_pc < excp_tbl.end_pc) {	// in the exception range.
			if (excp_tbl.catch_type == 0) {		// finally block. must be right.
#ifdef DEBUG
	sync_wcout{} << "which's holder is `any` --> finally block.[V]" << std::endl;
	sync_wcout{} << "===---------------- [End Finding Catch/Finally Block [V]] -----------------===" << std::endl;
#endif
				return excp_tbl.handler_pc;
			} else {		// catch block. should judge current exception type is the `catch_type` or not?
				auto _pair = (*rt_pool)[excp_tbl.catch_type - 1];
				assert(_pair.first == CONSTANT_Class);
				auto catch_klass = std::static_pointer_cast<InstanceKlass>(boost::any_cast<shared_ptr<Klass>>(_pair.second));
				if (cur_excp == catch_klass || cur_excp->check_interfaces(catch_klass) || cur_excp->check_parent(catch_klass)) {	// bug report: 卡了两天的 bug ！！卧槽......没有 catch 住 ClassNotFoundException，真正的原因在于没有判断此类是不是 catch_klass... 只判断 check_parent 和 check_interface 了...... 托这 bug 的福...... 把各种 ClassLoader 的源码翻了个遍...... 很有收获......
#ifdef DEBUG
	sync_wcout{} << "which's holder is `"<< excp_tbl.handler_pc << "` --> catch block.[V]" << std::endl;
	sync_wcout{} << "===---------------- [End Finding Catch/Finally Block [V]] -----------------===" << std::endl;
#endif
					return excp_tbl.handler_pc;
				}
			}
		}
#ifdef DEBUG
	sync_wcout{} << "not matched.[x]" << std::endl;
#endif
	}
	// failed. NO catch handler.
#ifdef DEBUG
	sync_wcout{} << "(DEBUG) didn't find a handler in this frame..." << std::endl;
	sync_wcout{} << "===---------------- [End Finding Catch/Finally Block...[x]] -----------------===" << std::endl;
#endif
	return 0;
}

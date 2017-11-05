#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <jarLister.hpp>
#include <utils.hpp>

using std::wcout;
using std::wcerr;
using std::endl;
using std::ifstream;
using std::wstringstream;
namespace bf = boost::filesystem;

using std::make_shared;

// suppose we have only Mac, Linux or Windows
#if (defined (__APPLE__) || defined (__linux__))
	wstring splitter(L"/");
#else
	wstring splitter(L"\\");
#endif

/*===---------------- RtJarDirectory --------------------*/
shared_ptr<RtJarDirectory> RtJarDirectory::findFolderInThis(const wstring & fdr_name) const	// fdr --> folder 
{
	if (this->subdir == nullptr) return nullptr;

	auto iter = this->subdir->find(make_shared<RtJarDirectory>(fdr_name));
	if (iter != this->subdir->end()) return *iter;
	else return nullptr;
}

void RtJarDirectory::add_file(StringSplitter && ss)
{
	if (ss.counter() == 0) {		// 仅仅在第一次的时候做检查，看文件到底存不存在
		if (this->find_file(std::move(ss)) == true) return;
		else ss.counter() = 0;
	}

	const wstring & target = ss.result()[ss.counter()];
	if (ss.counter() == ss.result().size() - 1) {	// next will be the target, add.
		subdir->insert(make_shared<RtJarDirectory>(target));
	} else {	// dir.
		auto next_dir = findFolderInThis(target);
		ss.counter() += 1;
		if (next_dir != nullptr) {
			(*next_dir).add_file(std::move(ss));	// delegate to the next level dir.
		} else {	// no next_dir, we should create.
			// this level's `subdir` can't be nullptr :)
			subdir->insert(make_shared<RtJarDirectory>(target));
			next_dir = findFolderInThis(target);
			assert(next_dir != nullptr);
			(*next_dir).add_file(std::move(ss));
		}
	}
}	

bool RtJarDirectory::find_file(StringSplitter && ss) const 
{
	const wstring & target = ss.result()[ss.counter()];
	// first check `target` is this a file && is this the true file.
	// if (ss.counter() == ss.result().size() && this->name == target)	return true;
	// second the `target` must be a dir. find in sub-dirs
	auto next_dir = findFolderInThis(target);
	if (next_dir != nullptr) {
		ss.counter() += 1;
		if (ss.counter() == ss.result().size())	return true;
		else return (*next_dir).find_file(std::move(ss));
	} else
		return false;
}

void RtJarDirectory::print() const
{
	#ifdef DEBUG
	std::cout << "*********************************" << endl;
	wcout.imbue(std::locale(""));
	this->print(0);
	std::cout << "*********************************" << endl;
	#endif
}

void RtJarDirectory::print(int level) const
{
	for(int i = 0; i < level; i ++)	std::cout << "\t";
	wcout << this->name << endl;
	if (this->subdir != nullptr) {
		for(auto & sub : *subdir) {
			(*sub).print(level+1);
		}
	}
}

/*===---------------- Filter ----------------------*/
const unordered_set<wstring> exclude_files{ L"META-INF/" };

/*===---------------- JarLister --------------------*/
bool JarLister::getjarlist(const wstring & rtjar_pos) const
{
	wstringstream cmd;
	cmd << L"jar tf " << rtjar_pos << L" > " << this->rtlist;
	int status =  system(wstring_to_utf8(cmd.str()).c_str());
	// TODO: judge whether mkdir is exist?
	if (bf::exists(uncompressed_dir)) {	// 如果存在
		return true;
	}
	cmd.str(L"");
	cmd << L"mkdir " << uncompressed_dir << L" > /dev/null 2>&1";
	system(wstring_to_utf8(cmd.str()).c_str());
	cmd.str(L"");
	std::cout << "unzipping rt.jar... please wait.\n";
	cmd << L"unzip " << rtjar_pos << L" -d " << uncompressed_dir << L" > /dev/null 2>&1";
	system(wstring_to_utf8(cmd.str()).c_str());
	if (status == -1) {  	// http://blog.csdn.net/cheyo/article/details/6595955 [shell 命令是否执行成功的判定]
		std::cerr << "system error!" << endl;
	} else {  
		if (WIFEXITED(status)) {  
			if (0 == WEXITSTATUS(status)) {  
				std::cout << "unzipping succeed.\n";
				return true;
			}  
			else {  
				std::cerr << "Your rt.jar file is not right!" << endl;  
			}  
		} else {  
			std::cerr << "other fault reasons!" << endl;
		}  
	}  
	return false;
}

JarLister::JarLister(const wstring & rtjar_pos) : rjd(L"root"), rtjar_pos(rtjar_pos)
{
	bool success = this->getjarlist(rtjar_pos);
	if (!success)	exit(-1);

	ifstream f(wstring_to_utf8(this->rtlist), std::ios_base::in);
	std::string s;
	while(!f.eof()) {
		f >> s;		// 这里有一个细节。因为最后一行仅仅有个回车，所以会读入空，也就是 s 还是原来的 s，即最后一个名字被读入了两遍。使用其他的方法对效率不好，因此在 add_file 中解决了。如果检测到有，忽略。
		if (!Filter::filt(utf8_to_wstring(s))) {
			this->rjd.add_file(StringSplitter(utf8_to_wstring(s)));
		}
	}
//	std::cout << "haha JarLister! " << this << std::endl;	// delete
//	this->rjd.print();		// delete.
//	sleep(3);				// delete.
}

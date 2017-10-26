#include <iostream>
#include <cstdlib>
#include <fstream>
#include <boost/filesystem.hpp>
#include <jarLister.hpp>

using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
namespace bf = boost::filesystem;

using std::make_shared;

// suppose we have only Mac, Linux or Windows
#if (defined (__APPLE__) || defined (__linux__))
	string splitter("/");
#else
	string splitter("\\");
#endif

/*===---------------- RtJarDirectory --------------------*/
shared_ptr<RtJarDirectory> RtJarDirectory::findFolderInThis(const string & fdr_name) const	// fdr --> folder 
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

	const string & target = ss.result()[ss.counter()];
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
	const string & target = ss.result()[ss.counter()];
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
	cout << "*********************************" << endl;
	this->print(0);
	cout << "*********************************" << endl;
	#endif
}

void RtJarDirectory::print(int level) const
{
	for(int i = 0; i < level; i ++)	cout << "\t";
	cout << this->name << endl;
	if (this->subdir != nullptr) {
		for(auto & sub : *subdir) {
			(*sub).print(level+1);
		}
	}
}

/*===---------------- Filter ----------------------*/
const unordered_set<string> exclude_files{ "META-INF/" };

/*===---------------- JarLister --------------------*/
bool JarLister::getjarlist(const string & rtjar_pos) const
{
	string cmd = "jar tf " + rtjar_pos + " > " + this->rtlist;
	int status =  system(cmd.c_str());
	// TODO: judge whether mkdir is exist?
	if (bf::exists(uncompressed_dir)) {	// 如果存在
		return true;
	}
	cmd = "mkdir " + uncompressed_dir + " > /dev/null 2>&1";
	system(cmd.c_str());
	cmd = "unzip " + rtjar_pos + " -d " + uncompressed_dir + " > /dev/null 2>&1";
	system(cmd.c_str());
	if (status == -1) {  	// http://blog.csdn.net/cheyo/article/details/6595955 [shell 命令是否执行成功的判定]
		cerr << "system error!" << endl;
	} else {  
		if (WIFEXITED(status)) {  
			if (0 == WEXITSTATUS(status)) {  
				return true;
			}  
			else {  
				cerr << "Your rt.jar file is not right!" << endl;  
			}  
		} else {  
			cerr << "other fault reasons!" << endl;
		}  
	}  
	return false;
}

JarLister::JarLister(const string & rtjar_pos) : rjd("root")
{
	bool success = this->getjarlist(rtjar_pos);
	if (!success)	exit(-1);

	ifstream f(this->rtlist, std::ios_base::in);
	string s;
	while(!f.eof()) {
		f >> s;		// 这里有一个细节。因为最后一行仅仅有个回车，所以会读入空，也就是 s 还是原来的 s，即最后一个名字被读入了两遍。使用其他的方法对效率不好，因此在 add_file 中解决了。如果检测到有，忽略。
		if (!Filter::filt(s)) {
			this->rjd.add_file(StringSplitter(s));
		}
	}
	this->rjd.print();
}
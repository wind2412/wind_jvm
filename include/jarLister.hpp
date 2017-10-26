#ifndef __JARLISTER_H__
#define __JARLISTER_H__

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>

using std::shared_ptr;
using std::wstring;
using std::set;
using std::vector;
using std::unordered_set;

extern wstring splitter;

#define DEBUG

class StringSplitter {
private:
	vector<wstring> splits;
	int cur = 0;	// counter for finding in RtJarDirectory
public:
	StringSplitter(const wstring &s) { boost::split(splits, s, boost::is_any_of(splitter)); }
	const vector<wstring>& result() { return splits; }
	int & counter() { return cur; }
};

class RtJarDirectory {
private:
	wstring name;
	shared_ptr<set<shared_ptr<RtJarDirectory>>> subdir;		// sub directory.
public:
	explicit RtJarDirectory(const wstring & filename) : name(filename) {
		if (boost::ends_with(filename, L".class"))	subdir = nullptr;
		else subdir.reset(new set<shared_ptr<RtJarDirectory>>);	// why shared_ptr cancelled the openator = ... emmmm...
	}
private:
	shared_ptr<RtJarDirectory> findFolderInThis(const wstring &s) const;
	void print(int level) const;
public:
	const wstring & get_name() const {return name;}
	// shared_ptr<RtJarDirectory> get_subdir(const string &filename);
	void add_file(StringSplitter && ss);
	bool find_file(StringSplitter && ss) const;
	void print() const;
public:
	bool operator < (const RtJarDirectory & rhs) {	// for set sort
		return this->name < rhs.name;
	}
	friend bool operator < (const shared_ptr<RtJarDirectory> &lhs, const shared_ptr<RtJarDirectory> &rhs) {
		return *lhs < *rhs;
	}
};

extern const unordered_set<wstring> exclude_files;

class Filter {
public:
	static bool filt(const wstring & s) {
		bool have1 = exclude_files.find(s) != exclude_files.end();
		bool have2 = false;
		for(const wstring & ss : exclude_files) {
			if(s.find(ss) != -1) {
				have2 = true;
				break;
			}
		}
		return have1 || have2;
	}
};

class JarLister {
private:
	wstring rtjar_pos;	// for cache.
	RtJarDirectory rjd;
	const wstring rtlist = L"rt.list";
	const wstring uncompressed_dir = L"sun_src";
private:
	// get rt.jar files list and put them into a file `rt.list`
	bool getjarlist(const std::wstring & rtjar_pos) const;
public:
	JarLister(const std::wstring & rtjar_pos = L"/Library/Java/JavaVirtualMachines/jdk1.8.0_111.jdk/Contents/Home/jre/lib/rt.jar");	// for Mac
	bool find_file(const std::wstring & classname) {	// java/util/Map.class
		return rjd.find_file(StringSplitter(classname));
	}
};



#endif
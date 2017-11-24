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

struct shared_RtJarDirectory_compare;

class RtJarDirectory {
private:
	wstring name;
	shared_ptr<set<shared_ptr<RtJarDirectory>, shared_RtJarDirectory_compare>> subdir;		// sub directory.
public:
	explicit RtJarDirectory(const wstring & filename);
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
	bool operator < (const RtJarDirectory & rhs) const {	// for set sort
		return this->name < rhs.name;
	}
	bool operator < (const wstring & rhs) const {	// for C++14 subdir.find(wstring)
		return this->name < rhs;
	}
	friend bool operator < (const wstring & lhs, const RtJarDirectory & rhs) {
		return lhs < rhs.name;
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
	wstring rtjar_pos;
	RtJarDirectory rjd;
	const wstring rtlist = L"rt.list";
	const wstring uncompressed_dir = L"sun_src";
private:
	// get rt.jar files list and put them into a file `rt.list`
	bool getjarlist(const std::wstring & rtjar_pos) const;
public:
	JarLister();
	bool find_file(const std::wstring & classname) {	// java/util/Map.class
		return rjd.find_file(StringSplitter(classname));
	}
	wstring get_sun_dir() { return uncompressed_dir; }
	void print() { rjd.print(); }
};



#endif

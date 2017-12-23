#include <iostream>
#include <cstdlib>

using namespace std;


class Junk {
private:
	int i;
	int j;
	double p;
public:
	Junk(int i, int j, double p) : i(i), j(j), p(p) {}	
};

class Person {
private:
	int ii;
	Junk *junks;
public:
	Person(int ii) : ii(ii), junks((Junk *)malloc(sizeof(Junk) * ii)){
		for (int i = 0; i < ii; i ++) {
			::new ((void *)&junks[i]) Junk(1,1,1.0);
		}
	}
	~Person() {
		for (int i = 0; i < ii; i ++) {
			junks[i].~Junk();
		}
		free(junks);
	}
};


int main(int argc, char *argv[]) 
{
//	Person *a = (Person *)malloc(sizeof (Person));
//	::new ((void *)a) Person(3);
//	a->~Person();
//	free(a);

	Person p(3);
	
}
class Person {
	int i;
}

interface Haha{
	Person p = new Person();
}

class Test1 extends Person implements Haha{
	
	int i;
	
	public void hh() {
		super.i = 4;
	}
	
	public static void main(String[] args) {
		Test1 u = new Test1();
		u.i = 3;
		u.hh();
		System.out.println(u.i);
	}
}
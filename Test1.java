class Person {
	static int i;
}

interface Haha{
	Person p = new Person();
}

class Test1 extends Person implements Haha{
	
	public void hh() {
		i = 4;
		super.i = 2;
		System.out.println(Person.i);
	}
	
	public static void main(String[] args) {
		Test1 u = new Test1();
		u.i = 3;
		u.hh();
		Person.i = 10;
		System.out.println(u.i);
		System.out.println(Person.i);
	}
}
class Test11 {
	public static void main(String[] args) throws InterruptedException {
//		Test7 t0 = new Test7();
//		Test7 t1 = new Test7();
//		Test7 t2 = new Test7();
//		t0.start();
//		t1.start();
//		t2.start();
		
		
		Thread t = new Thread( () -> System.out.println("In Java8, Lambda expression rocks !!") );
		t.start();
		Runnable x = () -> {  
					System.out.println("Hello, World!");   
				};  
		Thread m = new Thread(x);
		m.start();
		t.join();
		m.join();
		new Thread( () -> { for (int i = 0; i < 1000; i ++) System.out.println(i); } ).start();
	}
}
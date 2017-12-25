class Test17 extends Thread{
	
	@Override
	public synchronized void run() {
//		for (int i = 90; i < 95; i ++) {
//			System.out.println(i);
//			if (i == 93)
//				Thread.yield();
//		}
//		System.out.println(Thread.currentThread());
		System.out.println("...");
	}
	
	public static void main(String[] args) {
		
		Test17 t0 = new Test17();
//		Test17 t1 = new Test17();
//		Test17 t2 = new Test17();
//		Test17 t3 = new Test17();
//		Test17 t4 = new Test17();
		
		t0.start();
//		t1.start();
//		t2.start();
//		t3.start();
//		t4.start();
		
		
		throw new RuntimeException();
			
	}
}
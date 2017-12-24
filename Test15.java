public class Test15 extends Thread{

	@Override
	public synchronized void run() {
		for (int i = 90; i < 95; i ++) {
			System.out.println(i);
			if (i == 93)
				Thread.yield();
		}
		System.out.println(Thread.currentThread());
	}
	
	int [][][] b = null;
	int [][] a = null;
	
	public static void main(String[] args) throws Throwable {
		
		Test15 t0 = new Test15();
		Test15 t1 = new Test15();
		Test15 t2 = new Test15();
		Test15 t3 = new Test15();
		Test15 t4 = new Test15();
		Test15 t5 = new Test15();
		Test15 t6 = new Test15();
		Test15 t7 = new Test15();
		Test15 t8 = new Test15();
		Test15 t9 = new Test15();
		
		t0.start();
		t1.start();
		t2.start();
		t3.start();
		t4.start();
		t5.start();
		t6.start();
		t7.start();
		t8.start();
		t9.start();
	}
}
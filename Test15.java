public class Test15 extends Thread{

	static int i = 0;

	@Override
	public synchronized void run() {
		if (i < 10) {
			i ++;
			new Test15().start();
		}
		System.out.println(Thread.currentThread());
	}
	
	public static void main(String[] args) throws Throwable {
		
		Test15 t0 = new Test15();
		
		t0.start();
	}
}
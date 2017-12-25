public class Test16 extends Thread{		// 无限循环创建线程

	@Override
	public synchronized void run() {
		new Test16().start();
		System.out.println(Thread.currentThread());
	}
	
	public static void main(String[] args) throws Throwable {
		
		Test16 t0 = new Test16();
		
		t0.start();
	}
}
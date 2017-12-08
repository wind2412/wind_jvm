import java.io.FileInputStream;
import java.lang.reflect.Field;
import java.net.URLClassLoader;
import java.nio.ByteBuffer;
import java.util.concurrent.ConcurrentHashMap;

import sun.launcher.LauncherHelper;
import sun.misc.Launcher;
import sun.misc.Perf;
import sun.misc.Signal;
import sun.misc.SignalHandler;
import sun.misc.URLClassPath;
import sun.nio.fs.UnixFileSystemProvider;

// 粗略计算了一下。执行这个文件，需要执行 85w+ 个字节码。iterm 都被输出卡死了...

public class Test7 extends Thread{

	@Override
	public synchronized void run() {
		for (int i = 0; i < 20; i ++) {
			System.out.println(i);
			if (i == 10)
				Thread.yield();
		}
		System.out.println(Thread.currentThread());
		throw new RuntimeException();
	}
	
	int [][][] b = null;
	int [][] a = null;
	
	public static void main(String[] args) throws Throwable {
		
		Test7 t = new Test7();
		Test7 t1 = new Test7();
		Test7 t2 = new Test7();
		
//		t.start();
//		t1.start();
//		t2.start();
		
		
		Field[] field = Test7.class.getDeclaredFields();
		for (Field f : field) {
			System.out.println(f.getType());
		}
	
		
		Class clzz = int.class;
		System.out.println(clzz);
		

		Signal.handle(new Signal("INT"), new SignalHandler() {
            public void handle(Signal sig) {
                System.out
                        .println("Aaarggh, a user is trying to interrupt me!!");
                System.out
                        .println("(throw garlic at user, say `shoo, go away')");
            }
        });
		Signal.handle(new Signal("INT"), SignalHandler.SIG_IGN);
	
//		LauncherHelper
//		UnixFileSystem
		System.out.println(Float.POSITIVE_INFINITY / 0);
		System.out.println(-0.0f == 0.0f);
		
		String s = new String();
		
//		try {
//			s.wait();
//		} catch (InterruptedException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
		
		FileInputStream f = new FileInputStream("/");
		
		
		
//		System.out.println("haha");
		
	}
}
# wind jvm
My simple java virtual machine

## Environment Support
Mac, Linux. Windows is not supported, because the underlying os lib is os-related.
You'd use C++14 support and boost library support.
And most important, your **JDK8** java library. I only support java8.

## How to play
* If you're on mac, first you'd need a boost library. 
  1. If you don't have one, try `brew install boost` is okay.
  2. Then, you should modify the `Makefile`, and modify the linker command to your own boost library path.
  3. Then, you should modify the `config.xml`, and settle your jdk source code `rt.jar` path at the config.xml.
  4. Then, run `make test` to compile the Tests java code. Besides, Test7 must be compiled from debug version jvm. So I prepared one first.
  5. Run `make -j 8` to compile.
  6. Please **MUST** run at the root folder `wind_jvm`. Or path detection will lost accuracy and throw exception. So you must run `./bin/wind_jvm Test1` at the `wind_jvm` folder.
  7. Then have fun! Though this vm is simple and not complete, hope it can help you and gain interest~
* Linux is also okay~ only one is different from the mac version is, you should run something like `sudo apt-get install libboost-all-dev` on ubuntu.
  Besides, `config.xml` should be settled at the `<linux>...</linux>` space.

## Issue
If you have some issues or want to pull requests, welcome to submit!

## A Simple Example
At `wind_jvm` folder:
```
> ./bin/wind_jvm Test3
Hello, wind jvm!
```
## A Complex Example (Triggered gc)
Java Example: (Test7.java)
```Java
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

import java.lang.Void;

public class Test7 extends Thread{

	@Override
	public void run() {
		for (int i = 90; i < 95; i ++) {
			System.out.println(i);
			if (i == 93)
				Thread.yield();
		}
		System.out.println(Thread.currentThread());
		throw new RuntimeException();
	}
	
	int [][][] b = null;
	int [][] a = null;
	
	public static void main(String[] args) throws Throwable {
		
		Test7 t0 = new Test7();
		Test7 t1 = new Test7();
		Test7 t2 = new Test7();
		Test7 t3 = new Test7();
		Test7 t4 = new Test7();
		Test7 t5 = new Test7();
		Test7 t6 = new Test7();
		Test7 t7 = new Test7();
		Test7 t8 = new Test7();
		Test7 t9 = new Test7();
		
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
		
		Field[] field = Test7.class.getDeclaredFields();
		for (Field f : field) {
			System.out.println(f.getType());
		}
	
		
		Class clzz = int.class;
		System.out.println(clzz);
		Class clzz1 = void.class;
		System.out.println(clzz1);
		Class clzz2 = Void.class;
		System.out.println(clzz2);
		System.out.println(clzz1 == clzz2);
		

		Signal.handle(new Signal("INT"), new SignalHandler() {
          		public void handle(Signal sig) {
                		System.out.println("Will not get an output.");
            		}
          	});
		Signal.handle(new Signal("INT"), SignalHandler.SIG_IGN);

		System.out.println(Float.POSITIVE_INFINITY / 0);
		System.out.println(-0.0f == 0.0f);
		
		String s = new String();
		
		FileInputStream f = new FileInputStream("/");
	}
}
```
Then, After you compile it to `Test7.class`, You can:
At `wind_jvm` folder:
```
> ./bin/wind_jvm Test7
signal gc...
===------------- ThreadTable ----------------===
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[0]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[0]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
init_gc() over.
===------------- ThreadTable ----------------===
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[0]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[0]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c60a000], is the [5] thread, Thread Oop address: [140,440,780,852,720], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
===------------- ThreadTable ----------------===
pthread_t :[0x70000c68d000], is the [6] thread, Thread Oop address: [140,440,780,889,952], state:[0]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c60a000], is the [5] thread, Thread Oop address: [140,440,780,852,720], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
===------------- ThreadTbalbolcek !-
---------------===
pthread_t :[0x70000c710000], is the [7] thread, Thread Oop address: [140,440,780,927,184], state:[1]
pthread_t :[0x70000c68d000], is the [6] thread, Thread Oop address: [140,440,780,889,952], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c60a000], is the [5] thread, Thread Oop address: [140,440,780,852,720], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c793000], is the [8] thread, Thread Oop address: [140,440,780,964,416], state:[1]
pthread_t :[0x70000c816000], is the [9] thread, Thread Oop address: [140,440,781,001,648], state:[0]
pthread_t :[0x70000c710000], is the [7] thread, Thread Oop address: [140,440,780,927,184], state:[1]
pthread_t :[0x70000c68d000], is the [6] thread, Thread Oop address: [140,440,780,889,952], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c60a000], is the [5] thread, Thread Oop address: [140,440,780,852,720], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[0](main)
===------------------------------------------===
block!
block!
block!
===------------- ThreadTable ----------------===
pthread_t :[0x70000c899000], is the [10] thread, Thread Oop address: [140,440,781,038,880], state:[1]
pthread_t :[0x70000c793000], is the [8] thread, Thread Oop address: [140,440,780,964,416], state:[1]
pthread_t :[0x70000c816000], is the [9] thread, Thread Oop address: [140,440,781,001,648], state:[1]
pthread_t :[0x70000c710000], is the [7] thread, Thread Oop address: [140,440,780,927,184], state:[1]
pthread_t :[0x70000c68d000], is the [6] thread, Thread Oop address: [140,440,780,889,952], state:[1]
pthread_t :[0x70000c3fe000], is the [1] thread, Thread Oop address: [140,440,780,704,112], state:[1]
pthread_t :[0x70000c504000], is the [3] thread, Thread Oop address: [140,440,780,778,192], state:[1]
pthread_t :[0x70000c481000], is the [2] thread, Thread Oop address: [140,440,780,739,968], state:[1]
pthread_t :[0x70000c60a000], is the [5] thread, Thread Oop address: [140,440,780,852,720], state:[1]
pthread_t :[0x70000c587000], is the [4] thread, Thread Oop address: [140,440,780,815,488], state:[1]
pthread_t :[0x70000c37b000], is the [0] thread, Thread Oop address: [140,440,698,078,800], state:[1](main)
===------------------------------------------===
gc over!!
90                  // above is gc's output. I didn't delete it. From now on, it's the java program's output.
90
90
90
90
90
90
90
90
90
class [[I
91
91
91
91
91
91
91
91
91
91
class [[[I
92
92
92
92
92
92
92
92
92
92
int
93
93
93
93
93
93
93
93
93
93
void
94
94
94
94
94
94
94
94
94
94
class java.lang.Void
Thread[Thread-0,5,main]
Thread[Thread-3,5,main]
Exception in thread "Thread-0" Thread[Thread-6,5,main]    // MultiThread's output will be chaos. It's normal.
java.lang.RuntimeExceptionThread[Thread-4,5,main]

Thread[Thread-7,5,main]
Thread[Thread-9,5,main]	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)

Thread[Thread-8,5,main]
Thread[Thread-2,5,main]
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)Thread[Thread-5,5,main]

Thread[Thread-1,5,main]
false	at java/lang/Throwable.<init>(Throwable.java:249)

	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
Exception in thread "Thread-3" Exception in thread "Thread-6" Exception in thread "Thread-4" Exception in thread "Thread-7" Exception in thread "Thread-9" Exception in thread "Thread-8" Exception in thread "Thread-2" Exception in thread "Thread-5" Exception in thread "Thread-1" java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
2.13909504E9
	at Test7.run(Test7.java:23)
true
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
java.lang.RuntimeException
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:249)
	at java/lang/Exception.<init>(Exception.java:54)
	at java/lang/RuntimeException.<init>(RuntimeException.java:51)
	at Test7.run(Test7.java:23)
Exception in thread "main" java.io.FileNotFoundException: / (Is a directory)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:0)
	at java/lang/Throwable.fillInStackTrace(Throwable.java:781)
	at java/lang/Throwable.<init>(Throwable.java:264)
	at java/lang/Exception.<init>(Exception.java:66)
	at java/io/IOException.<init>(IOException.java:58)
	at java/io/FileNotFoundException.<init>(FileNotFoundException.java:64)
	at java/io/FileInputStream.open0(FileInputStream.java:0)
	at java/io/FileInputStream.open(FileInputStream.java:195)
	at java/io/FileInputStream.<init>(FileInputStream.java:123)
	at java/io/FileInputStream.<init>(FileInputStream.java:93)
	at Test7.main(Test7.java:37)
```

## About Tests
Some Java Tests are from network, especially those java lambda test files. Thank you~

## pitfalls that can improve
1. NAN's precise message
2. exception's precise message
3. improve lambda

## Java Features Support
- [x] fully independent ClassFile parser tool support: @[wind2412:javap](https://github.com/wind2412/javap) 
- [x] multithread support
- [x] exception support
- [x] some native libraries support
- [x] simple lambda support (invokedynamic)
- [x] big deal of reflection support
- [x] stop-the-world and GC support, using GC-Root algorithm.

## Output bytecode execution messages
If you modify `Makefile` and modify it to `CPP_FLAGS := -std=c++14 -O3 -DDEBUG -DKLASS_DEBUG -DPOOL_DEBUG -DSTRING_DEBUG`, all execution message will be showed in output.    

```
// Test.java
class Test {
	public static void main(String[] args) {
		System.out.println("Today is a good day！On December 2nd，my jvm can print hello world~\n");	
	}
}
```

```
// re-compile and re-run:
> ./bin/wind_jvm Test
```

You can see lots of bytecode output. And the execution result are as below:
![hello world](https://wind2412.files.wordpress.com/2017/12/qq20171231-1436422x.jpg)

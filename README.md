# wind jvm
My simple java virtual machine

## Environment Support

- **Supported Platforms**: macOS, Linux
- **Not Supported**: Windows (due to OS-specific dependencies in the underlying libraries)
- **Requirements**:
  - **C++14**
  - **Boost Library**
  - **JDK 8** (specifically `rt.jar` from Java 8)

## Getting Started

### macOS

1. **Install Boost Library**:  
   If Boost is not installed, run the following command:  
   ```bash
   brew install boost
   ```

2. **Modify the Makefile**:  
   Update the linker command to use your local Boost library path.  

3. **Configure `config.xml`**:  
   Set the path to your JDK 8's `rt.jar` in the `config.xml` file.

4. **Compile Tests**:  
   Run the following command to compile the test Java code:  
   ```bash
   make test
   ```
   _Note: `Test7` must be compiled using a debug version JVM. A precompiled classfile is already provided._

5. **Build the Project**:  
   Use the following command to compile the project:  
   ```bash
   make -j 8
   ```

6. **Run Wind JVM**:  
   Navigate to the root directory of `wind_jvm` and execute the following command:  
   ```bash
   ./bin/wind_jvm Test1
   ```
   _Important: You **must** run the JVM from the root directory, or path detection may fail and cause exceptions._

7. **Enjoy!**  
   While this JVM is simple, it’s a fun project that may help spark your interest in virtual machine development.

---

### Linux

1. **Install Boost Library**:  
   Run the following command to install Boost:  
   ```bash
   sudo apt-get install libboost-all-dev
   ```

2. **Configure `config.xml`**:  
   Set the `<linux>` section in `config.xml` to point to your JDK 8's `rt.jar` path.

3. Follow steps 4–6 in the macOS section above.

---

## Usage Examples

### Simple Example
From the `wind_jvm` folder, run [Test3](https://github.com/wind2412/wind_jvm/blob/master/Test3.java):  
```bash
./bin/wind_jvm Test3
```

Output:  
```
Hello, wind jvm!
```

### Complex Example (Triggering GC)
From the `wind_jvm` folder, run [Test7](https://github.com/wind2412/wind_jvm/blob/master/Test7.java):  
```bash
> ./bin/wind_jvm Test7
```

Sample output is (with internal debugging messages):
```
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

Some Java test files, especially those for Java lambda expressions, were sourced from online examples.  

---

## Pitfalls and Areas for Improvement

1. Improve the precision of **NAN** error messages.  
2. Enhance the clarity of exception messages.  
3. Add support for more complex lambda expressions.

---

## Java Features Support

- [x] Fully independent ClassFile parser tool support: [@wind2412:javap](https://github.com/wind2412/javap)  
- [x] Multithread support  
- [x] Exception handling support  
- [x] Some native libraries support  
- [x] Simple lambda support (via `invokedynamic`)  
- [x] Reflection support for most scenarios  
- [x] Stop-the-world and GC support using the GC-Root algorithm

---

## Debugging Bytecode Execution

If you want to see detailed bytecode execution messages, modify the `Makefile` as follows:

```makefile
CPP_FLAGS := -std=c++14 -O3 -DDEBUG -DKLASS_DEBUG -DPOOL_DEBUG -DSTRING_DEBUG
```

After recompiling, running the JVM will display detailed execution messages.

### Example Code
```java
// Test.java
class Test {
    public static void main(String[] args) {
        System.out.println("Today is a good day! On December 2nd, my JVM can print hello world~\n");
    }
}
```

### Running the Code
Re-compile and run:
```bash
./bin/wind_jvm Test
```

### Output Example
You will see detailed bytecode output and the execution result:  

```
Hello, wind jvm!
```
![hello world](https://wind2412.files.wordpress.com/2017/12/qq20171231-1436422x.jpg)


This project was created during my undergraduate studies, around 2017–2018.

Thank you for playing with wind JVM! Though it’s simple, I hope you enjoy exploring it and find it helpful for learning about JVM internals.


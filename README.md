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

## About Tests
Some Java Tests are from network, especially those java lambda test files. Thank you~

## Java Features Support
- [x] fully independent ClassFile parser tool support: @[wind2412:javap](https://github.com/wind2412/javap) 
- [x] multithread support
- [x] exception support
- [x] some native libraries support
- [x] simple lambda support (invokedynamic)
- [x] big deal of reflection support
- [x] stop-the-world and GC support, using GC-Root algorithm.

## Welcome to communicate with me~

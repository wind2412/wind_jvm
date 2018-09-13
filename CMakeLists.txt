cmake_minimum_required(VERSION 3.5)
project(wind_jvm)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(INCLUDE_LIST
        include/native/java_io_FileDescriptor.hpp
        include/native/java_io_FileInputStream.hpp
        include/native/java_io_FileOutputStream.hpp
        include/native/java_io_FileSystem.hpp
        include/native/java_io_UnixFileSystem.hpp
        include/native/java_lang_Class.hpp
        include/native/java_lang_ClassLoader.hpp
        include/native/java_lang_Double.hpp
        include/native/java_lang_Float.hpp
        include/native/java_lang_invoke_MethodHandle.hpp
        include/native/java_lang_invoke_MethodHandleNatives.hpp
        include/native/java_lang_Object.hpp
        include/native/java_lang_Package.hpp
        include/native/java_lang_reflect_Array.hpp
        include/native/java_lang_Runtime.hpp
        include/native/java_lang_Shutdown.hpp
        include/native/java_lang_String.hpp
        include/native/java_lang_System.hpp
        include/native/java_lang_Thread.hpp
        include/native/java_lang_Throwable.hpp
        include/native/java_security_AccessController.hpp
        include/native/java_util_concurrent_atomic_AtomicLong.hpp
        include/native/native.hpp
        include/native/sun_misc_signal.hpp
        include/native/sun_misc_Unsafe.hpp
        include/native/sun_misc_URLClassPath.hpp
        include/native/sun_misc_VM.hpp
        include/native/sun_reflect_NativeConstructorAccessorImpl.hpp
        include/native/sun_reflect_NativeMethodAccessorImpl.hpp
        include/native/sun_reflect_Reflection.hpp
        include/runtime/annotation.hpp
        include/runtime/bytecodeEngine.hpp
        include/runtime/constantpool.hpp
        include/runtime/field.hpp
        include/runtime/gc.hpp
        include/runtime/klass.hpp
        include/runtime/method.hpp
        include/runtime/oop.hpp
        include/runtime/thread.hpp
        include/utils/lock.hpp
        include/utils/monitor.hpp
        include/utils/os.hpp
        include/utils/synchronize_wcout.hpp
        include/utils/utils.hpp
        include/class_parser.hpp
        include/classloader.hpp
        include/jarLister.hpp
        include/system_directory.hpp
        include/wind_jvm.hpp)
set(SRC_LIST
        src/native/java_io_FileDescriptor.cpp
        src/native/java_io_FileInputStream.cpp
        src/native/java_io_FileOutputStream.cpp
        src/native/java_io_FileSystem.cpp
        src/native/java_io_UnixFileSystem.cpp
        src/native/java_lang_Class.cpp
        src/native/java_lang_ClassLoader.cpp
        src/native/java_lang_Double.cpp
        src/native/java_lang_Float.cpp
        src/native/java_lang_invoke_MethodHandle.cpp
        src/native/java_lang_invoke_MethodHandleNatives.cpp
        src/native/java_lang_Object.cpp
        src/native/java_lang_Package.cpp
        src/native/java_lang_reflect_Array.cpp
        src/native/java_lang_Runtime.cpp
        src/native/java_lang_Shutdown.cpp
        src/native/java_lang_String.cpp
        src/native/java_lang_System.cpp
        src/native/java_lang_Thread.cpp
        src/native/java_lang_Throwable.cpp
        src/native/java_security_AccessController.cpp
        src/native/java_util_concurrent_atomic_AtomicLong.cpp
        src/native/native.cpp
        src/native/sun_misc_signal.cpp
        src/native/sun_misc_Unsafe.cpp
        src/native/sun_misc_URLClassPath.cpp
        src/native/sun_misc_VM.cpp
        src/native/sun_reflect_NativeConstructorAccessorImpl.cpp
        src/native/sun_reflect_NativeMethodAccessorImpl.cpp
        src/native/sun_reflect_Reflection.cpp
        src/runtime/annotation.cpp
        src/runtime/bytecodeEngine.cpp
        src/runtime/constantpool.cpp
        src/runtime/field.cpp
        src/runtime/gc.cpp
        src/runtime/klass.cpp
        src/runtime/method.cpp
        src/runtime/oop.cpp
        src/runtime/thread.cpp
        src/utils/lock.cpp
        src/utils/os.cpp
        src/utils/synchronize_wcout.cpp
        src/utils/utils.cpp
        src/class_parser.cpp
        src/classloader.cpp
        src/jarLister.cpp
        src/main.cpp
        src/system_directory.cpp
        src/wind_jvm.cpp
#        tests/testClassParser.cpp
#        tests/testJarLister.cpp
#        tests/testRtJarDirectory.cpp
#        useful_tools/classfile_interceptor.cpp
        )

include_directories(include)
include_directories(include/native)
include_directories(include/runtime)
include_directories(include/utils)

add_definitions(-Og -g)

add_executable(wind_jvm ${INCLUDE_LIST} ${SRC_LIST})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    link_directories(/usr/local/Cellar/boost/1.60.0_2/lib/)
    target_link_libraries(wind_jvm -lboost_filesystem -lboost_system)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    link_directories(/usr/lib/x86_64-linux-gnu/)
    target_link_libraries(wind_jvm -lpthread -lboost_filesystem -lboost_system)
endif()



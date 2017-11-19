/*
 * jni.hpp
 *
 *  Created on: 2017年11月19日
 *      Author: zhengxiaolin
 */

#ifndef INCLUDE_NATIVE_JNI_HPP_
#define INCLUDE_NATIVE_JNI_HPP_

#include <string>

using std::string;

// Java Native Interface Spec. [$ JNI types]
// ps: this jni is not real jni. It's target is only to support Native Method.

// from hotspot:
typedef struct {
    string name;
    string signature;
    void *fnPtr;
} JNINativeMethod;

#define THD "Ljava/lang/Thread;"
#define OBJ "Ljava/lang/Object;"
#define STE "Ljava/lang/StackTraceElement;"
#define STR "Ljava/lang/String;"
#define IPS "Ljava/io/InputStream;"
#define PRS "Ljava/io/PrintStream;"
#define PRO "Ljava/util/Properties;"
#define KLS "Ljava/lang/Class;"




#endif /* INCLUDE_NATIVE_JNI_HPP_ */

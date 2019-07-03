/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include "com_here_android_olp_TesterActivity.h"

#include <jni.h>
#include <android/log.h>
#include <gtest/gtest.h>
#include <fstream>
#include <thread>
#include <vector>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <olp/core/context/Context.h>
#include <olp/core/porting/make_unique.h>
#include <testutils/CustomParameters.hpp>

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))

std::thread* threadPtr = nullptr;
std::atomic<bool> isFinished(false);

void redirectStdout() {
    using namespace std;

    __android_log_print(ANDROID_LOG_INFO, "HEREOS_STDOUT", "Setting up STDOUT pipe to adb logcat");

    int stdoutPipe[2];
    pipe(stdoutPipe);

    dup2(stdoutPipe[1], STDOUT_FILENO);
    FILE *fd = fdopen(stdoutPipe[0], "r");

    stringstream outstream;
    int c;

    while (!isFinished)
    {
        c = fgetc(fd);
        if (c == '\n') {
            __android_log_print(ANDROID_LOG_INFO, "HEREOS_STDOUT", "%s", outstream.str().c_str());
            outstream.str("");
        } else {
            outstream << (char) c;
        }
    };

    __android_log_print(ANDROID_LOG_INFO, "HEREOS_STDOUT", "Closed STDOUT pipe to adb logcat");
}

void logcatSetup() {
    isFinished = false;
    if (!threadPtr) {
        threadPtr = new std::thread(redirectStdout);
        usleep(500);
        fflush(stdout);
    }
}

void logcatClose() {
    // Allow gtest to finalize
    usleep(2000);

    // End logcat thread, last char to stdout needed to unblock fgetc()
    isFinished = true;
    std::cout << EOF << std::endl;
    fflush(stdout);

    threadPtr->join();
    delete threadPtr;
    threadPtr = nullptr;
}

static void convertArgs(JNIEnv *env, jobjectArray java_args, std::vector<char *> &args)
{
    int java_argcount = env->GetArrayLength(java_args);
    for (int i = 0; i < java_argcount; ++i) {
        jstring string = static_cast<jstring>(env->GetObjectArrayElement(java_args, i));
        const char *java_arg = env->GetStringUTFChars(string, 0);
        args.push_back(::strdup(java_arg));
        env->ReleaseStringUTFChars(string, java_arg);
        env->DeleteLocalRef(string);
    }
    args.push_back(0);
}

static JavaVM *gVM;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_here_android_olp_TesterActivity_runTests(JNIEnv* env, jobject obj, jstring java_appPath, jobjectArray java_args)
{
    LOGI("runTests");

    // Get Context
    jclass clazz = env->GetObjectClass(obj);
    if (!clazz || env->ExceptionOccurred()) {
        LOGE("runTests failed to get class for object");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return JNI_FALSE;
    }

    jmethodID getContext = env->GetMethodID(clazz, "getContext", "()Landroid/content/Context;");
    if (!getContext || env->ExceptionOccurred()) {
        LOGE("runTests failed to get getContext method");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return JNI_FALSE;
    }

    jobject context = env->CallObjectMethod(obj, getContext);
    if (!context || env->ExceptionOccurred()) {
        LOGE("runTests failed to get Context");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return JNI_FALSE;
    }

    const char *appPath = env->GetStringUTFChars(java_appPath, 0);
    env->ReleaseStringUTFChars(java_appPath, appPath);

    std::vector<char *> args;
    convertArgs(env, java_args, args);
    std::vector<char *> original_args = args;

    char **argv = args.data();
    int argc = args.size() - 1;

    testing::InitGoogleTest(&argc, argv);

    CustomParameters::getInstance( ).init( argc, argv );

    logcatSetup();
    int result = RUN_ALL_TESTS();
    logcatClose();

    for (unsigned int i = 0; i < original_args.size() - 1; ++i) {
        ::free(args[i]);
    }

    LOGI("result=%d", result);
    return (result == 0) ? JNI_TRUE : JNI_FALSE;
}

std::unique_ptr<olp::context::Context::Scope> network_context;

extern "C"
JNIEXPORT void JNICALL
Java_com_here_android_olp_TesterActivity_setUpNative(JNIEnv* /*env*/, jobject /*obj*/,
                                                       jobject context)
{
    network_context = std::make_unique<olp::context::Context::Scope>( gVM, context );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_here_android_olp_TesterActivity_tearDownNative(JNIEnv* /*env*/, jobject /*obj*/)
{
    network_context.reset();
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    LOGI("JNI_OnLoad");
    gVM = vm;

#ifdef INIT_CPPREST
    cpprest_init(vm);
#endif

    return JNI_VERSION_1_6;
}

#include "FilePicker.hpp"

#ifdef __ANDROID__
#include <jni.h>
#include <android_native_app_glue.h>
extern android_app* g_AndroidApp;
#endif

namespace OxygenCrate::Platform
{
#ifdef __ANDROID__
    namespace
    {
        struct AndroidJniEnvScope
        {
            JNIEnv* Env = nullptr;
            JavaVM* Vm = nullptr;
            bool Attached = false;
        };

        bool AcquireJniEnv(AndroidJniEnvScope& scope)
        {
            if (!g_AndroidApp || !g_AndroidApp->activity || !g_AndroidApp->activity->vm)
                return false;

            scope.Vm = g_AndroidApp->activity->vm;
            jint status = scope.Vm->GetEnv(reinterpret_cast<void**>(&scope.Env), JNI_VERSION_1_6);
            if (status == JNI_EDETACHED)
            {
                if (scope.Vm->AttachCurrentThread(&scope.Env, nullptr) != JNI_OK)
                    return false;
                scope.Attached = true;
            }
            else if (status != JNI_OK)
            {
                return false;
            }

            return scope.Env != nullptr;
        }

        void ReleaseJniEnv(AndroidJniEnvScope& scope)
        {
            if (scope.Attached && scope.Vm)
                scope.Vm->DetachCurrentThread();
            scope.Env = nullptr;
            scope.Vm = nullptr;
            scope.Attached = false;
        }
    }

    void RequestFilePicker()
    {
        AndroidJniEnvScope scope;
        if (!AcquireJniEnv(scope))
            return;

        JNIEnv* env = scope.Env;
        jclass activityClass = env->GetObjectClass(g_AndroidApp->activity->clazz);
        if (activityClass)
        {
            jmethodID method = env->GetMethodID(activityClass, "openFilePicker", "()V");
            if (method)
                env->CallVoidMethod(g_AndroidApp->activity->clazz, method);
            env->DeleteLocalRef(activityClass);
        }

        if (env->ExceptionCheck())
            env->ExceptionClear();

        ReleaseJniEnv(scope);
    }

    std::string PollSelectedFile()
    {
        AndroidJniEnvScope scope;
        if (!AcquireJniEnv(scope))
            return {};

        std::string result;
        JNIEnv* env = scope.Env;
        jclass activityClass = env->GetObjectClass(g_AndroidApp->activity->clazz);
        if (activityClass)
        {
            jmethodID method = env->GetMethodID(activityClass, "pollSelectedFile", "()Ljava/lang/String;");
            if (method)
            {
                jstring jpath = static_cast<jstring>(env->CallObjectMethod(g_AndroidApp->activity->clazz, method));
                if (jpath)
                {
                    const char* chars = env->GetStringUTFChars(jpath, nullptr);
                    if (chars)
                    {
                        result = chars;
                        env->ReleaseStringUTFChars(jpath, chars);
                    }
                    env->DeleteLocalRef(jpath);
                }
            }
            env->DeleteLocalRef(activityClass);
        }

        if (env->ExceptionCheck())
            env->ExceptionClear();

        ReleaseJniEnv(scope);
        return result;
    }
#else
    void RequestFilePicker() {}
    std::string PollSelectedFile() { return {}; }
#endif
}

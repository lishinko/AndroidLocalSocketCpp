#include <jni.h>
#include <string>
#include <android/log.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <GLES3/gl32.h>
#include <GLES/egl.h>
#include "NativeEngine.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include "SingleThreadTask.h"
#include "GLTextureInterface.h"
#include <memory>
#include <chrono>

#include <android/bitmap.h>

#define LOG_TAG "LocalSocketServer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testandroidcpp_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */)
{
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
// 注意点：1，不考虑字符串结尾的\0，2，bind和sendto中的地址长度，应该是真实地址长度，不能设置为sizeof(server_addr)，
//  此2点问题参考下面的链接：
// https://stackoverflow.com/questions/14643571/localsocket-communication-with-unix-domain-in-android-ndk
static int GetLengthOfAbstractSocketName()
{
    return 0;
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_testandroidcpp_MainActivity_testLocalSocketServer(JNIEnv *env, jobject thiz)
{
}

static std::unique_ptr<saturnv::NativeEngine> s_engine;

static std::atomic_bool s_testThreadRunning;
static std::thread s_testThread;
static std::unique_ptr<saturnv::SingleThreadTask> s_taskManager;

static const int Width = 1920;
static const int Height = 1536;

extern "C" int32_t enqueueTask(HandleTask *handleCallback, DisposeTaskData *disposeCallback, TaskData data)
{
    if (s_taskManager != nullptr)
    {
        s_taskManager->Enqueue([handleCallback, disposeCallback, data]()
                               {
            if(handleCallback != nullptr){
                auto ret = (*handleCallback)(data);
                if(ret != 0)
                {
                    LOGE("%s:%d :  handleCallback !, ret = %d", __FILE_NAME__, __LINE__, ret);
                }
                if(disposeCallback){
                    (*disposeCallback)(data);
                }
                return;
            }
            else{
                LOGE("%s:%d :  handleCallback == nullptr!!", __FILE_NAME__, __LINE__);
                return;
            } });
        return 0;
    }
    return -1;
}
//将java的bitmap放到opengl线程并更新opengl贴图。
//textureId : 0 1 2 3 = 前左右后。
extern "C" JNIEXPORT void JNICALL
Java_com_hobot_saturnv_avm_unity_UnityGLJni_processBitmap(JNIEnv *env, jclass clazz, jobject bitmap,
                                                          jint tex_id) {
    if (s_taskManager == nullptr)
    {
        LOGE("%s:%d : processBitmap: s_taskManager == nullptr!", __FILE_NAME__, __LINE__);
        return;
    }
    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);
    auto* globalBitmap = env->NewGlobalRef(bitmap);
    s_taskManager->Enqueue([jvm, globalBitmap, tex_id]()
   {
        s_taskManager->EnsureAttachJvm(jvm);
        auto newEnv = s_taskManager->GetThreadEnv();
        if(newEnv == nullptr)
        {
            LOGE("%s:%d : processBitmap: env == nullptr!", __FILE_NAME__, __LINE__);
            return;
        }
        AndroidBitmapInfo info;
        void *pixels;
        int ret;

        // 获取 Bitmap 信息
        if ((ret = AndroidBitmap_getInfo(newEnv, globalBitmap, &info)) < 0)
        {
            return;
        }

        // 锁定 Bitmap，获取像素数据
        if ((ret = AndroidBitmap_lockPixels(newEnv, globalBitmap, &pixels)) < 0)
        {
            return;
        }
        //这段代码在opengl线程执行!
        if(s_engine != nullptr){
            auto id = s_engine->GetTextureId(tex_id);
            auto bytes = static_cast<const char*>(pixels);
            s_engine->UpdateTexture(id, bytes, info.width * info.height * 4);
        }
        // 解锁 Bitmap
        AndroidBitmap_unlockPixels(newEnv, globalBitmap);
        newEnv->DeleteGlobalRef(globalBitmap);
   });
}
// 开一个新线程，向opengl线程刷新数据
// 注意该线程既不是opengl线程也不是unity线程，可以用java线程代替
extern "C" void startTestThread()
{
    LOGI("%s:%d : startTestThread !, ", __FILE_NAME__, __LINE__);
    if (s_testThread.joinable())
    {
        LOGE("%s:%d : already run !, ", __FILE_NAME__, __LINE__);
        return;
    }
    s_testThreadRunning = true;
    s_testThread = std::thread([]()
                               {
        int value = 1;
        while (s_taskManager != nullptr && s_testThreadRunning)
        {
            s_taskManager->Enqueue([value](){
                //这段代码在opengl线程执行!
                if(s_engine != nullptr){
                    std::vector<char> pixels(Width * Height * 4);
                    memset(pixels.data(),value,pixels.size() );
                    auto* bytes = pixels.data();
                    LOGI("VALUE  = %d", value);
                    auto id = s_engine->GetTextureId(0);
                    s_engine->UpdateTexture(id,bytes, pixels.size());
                }
            });
            value += 10;
            if(value >= 255)
            {
                value = 10;
            }
            std::chrono::milliseconds dur(30);
            std::this_thread::sleep_for(dur);
        } });
}

extern "C" bool serviceRunning()
{
    return s_engine != nullptr && s_engine->Running_AnyThread();
}
extern "C" void stopTestThread()
{
    LOGI("%s:%d : stopTestThread !, ", __FILE_NAME__, __LINE__);
    s_testThreadRunning = false;
    if (s_testThread.joinable())
    {
        s_testThread.join();
    }
}
extern "C" uint32_t getTexture(int32_t id)
{
    LOGI("%s:%d : getTexture !, id = %d", __FILE_NAME__, __LINE__, id);
    if (s_taskManager != nullptr && s_engine != nullptr)
    {
        auto ret = s_engine->GetTextureId(id);
        LOGI("%s:%d : getTexture !, id = %d, ret = %d", __FILE_NAME__, __LINE__, id, ret);
        return ret;
    }
    return 0;
}
extern "C" void startService()
{
    LOGI("%s:%d : startService !, ", __FILE_NAME__, __LINE__);
    if (s_taskManager != nullptr)
    {
        LOGE("%s:%d : already inited !, ", __FILE_NAME__, __LINE__);
        return;
    }

    auto c = eglGetCurrentContext();
    if (c == nullptr)
    {
        LOGE("eglGetCurrentContext failed!");
        return;
    }
    auto d = eglGetCurrentDisplay();
    if (d == nullptr)
    {
        LOGE("%s:%d : eglGetCurrentDisplay failed!, error = %d", __FILE_NAME__, __LINE__, eglGetError());
        return;
    }
    EGLConfig config;
    int numConfig = 1;
    if (!eglGetConfigs(d, &config, 1, &numConfig))
    {
        LOGE("%s:%d : error = %d: eglGetConfigs failed!", __FILE_NAME__, __LINE__, eglGetError());
        return;
    }
    const GLubyte *v = glGetString(GL_VERSION);
    LOGI("%s:%d : error = %d: version = %s!", __FILE_NAME__, __LINE__, eglGetError(), v);

    if (s_taskManager == nullptr)
    {
        s_taskManager = std::make_unique<saturnv::SingleThreadTask>(true);
    }
    s_taskManager->Enqueue([c, config]()
                           {
        s_engine = std::make_unique<saturnv::NativeEngine>(c, config);
        if(!s_engine->InitOpenGL()){
            LOGE("%s:%d : error = %d: s_engine->InitOpenGL() failed!", __FILE_NAME__, __LINE__, eglGetError());
            return;
        }
        if(!s_engine->CreateTexture(4))
        {
            return;
        } });
}
extern "C" void stopService()
{
    LOGI("%s:%d : stopService !, ", __FILE_NAME__, __LINE__);
    if (s_taskManager != nullptr)
    {
        s_taskManager->Enqueue([]()
                               {
            //opengl析构函数应该在自己的线程中执行。
            if(s_engine){
                s_engine.release();
            }
            s_engine = nullptr; });
        s_taskManager.release();
    }
    s_taskManager = nullptr;
}

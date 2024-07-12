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

#define LOG_TAG "LocalSocketServer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring
JNICALL
Java_com_example_testandroidcpp_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
//注意点：1，不考虑字符串结尾的\0，2，bind和sendto中的地址长度，应该是真实地址长度，不能设置为sizeof(server_addr)，
// 此2点问题参考下面的链接：
//https://stackoverflow.com/questions/14643571/localsocket-communication-with-unix-domain-in-android-ndk
static int GetLengthOfAbstractSocketName(){
    return  0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_testandroidcpp_MainActivity_testLocalSocketServer(JNIEnv *env, jobject thiz) {

}

static saturnv::NativeEngine* s_engine;

static std::queue<std::function<void()>> s_tasks;
static std::mutex s_tasksLock;

static std::thread s_glThread;
static std::atomic_bool s_glThreadRunning = false;
static std::atomic_bool s_testThreadRunning = false;
static std::thread s_testThread;
static std::condition_variable s_taskVar;
static std::mutex s_taskVarLock;

static const int Width = 1920;
static const int Height = 1536;

void startTestThread(){
    if(s_testThread.joinable())
    {
        LOGE("%s:%d : already run !, ", __FILE_NAME__, __LINE__);
    }
    s_testThread = std::thread([](){
        int value = 1;
        while (s_glThreadRunning && s_testThreadRunning)
        {
            s_tasksLock.lock();
            s_tasks.push([value](){
                //这段代码在opengl线程执行!
                if(s_engine != nullptr){
                    std::vector<char> pixels(Width * Height * 4);
                    memset(pixels.data(),value,pixels.size() );
                    auto* bytes = pixels.data();
                    s_engine->UpdateTexture(0,bytes, pixels.size());
                }
            });
            s_tasksLock.unlock();

            s_taskVar.notify_one();
            std::chrono::milliseconds dur(30);
            std::this_thread::sleep_for(dur);
            value++;
            if(value >= 255)
            {
                value = 1;
            }
        }
    });
}
void stopTestThread(){
    s_testThreadRunning = false;
    if(s_testThread.joinable()){
        s_testThread.join();
    }
}
GLuint getTexture(int id){
    if(s_glThreadRunning && s_engine != nullptr)
    {
        return  s_engine->GetTextureId(id);
    }
    return 0;
}
void startService(){
    if(s_glThreadRunning){
        LOGE("%s:%d : already inited !, ", __FILE_NAME__, __LINE__);
        return;
    }

    auto c =  eglGetCurrentContext();
    if(c == nullptr){
        LOGE("eglGetCurrentContext failed!");
        return;
    }
    auto d = eglGetCurrentDisplay();
    if(d == nullptr){
        LOGE("%s:%d : eglGetCurrentDisplay failed!, error = %d", __FILE_NAME__, __LINE__, eglGetError());
        return;
    }
    EGLConfig config;
    int numConfig = 1;
    if(!eglGetConfigs(d,&config, 1,&numConfig )){
        LOGE("%s:%d : error = %d: eglGetConfigs failed!", __FILE_NAME__, __LINE__, eglGetError());
        return;
    }
    const GLubyte* v = glGetString(GL_VERSION);
    LOGI("%s:%d : error = %d: version = %s!", __FILE_NAME__, __LINE__, eglGetError(), v);

    s_glThreadRunning = true;
    s_glThread = std::thread([c, config](){
        s_engine = new saturnv::NativeEngine(c, config);
        if(s_glThreadRunning)
        {
            if(!s_engine->InitOpenGL()){
                LOGE("%s:%d : error = %d: s_engine->InitOpenGL() failed!", __FILE_NAME__, __LINE__, eglGetError());
                return;
            }
            if(!s_engine->CreateTexture(4))
            {
                return;
            }
            std::unique_lock lock(s_taskVarLock);
            while (s_glThreadRunning)
            {
                std::chrono::milliseconds waitDur(1000);
                s_taskVar.wait_for(lock,waitDur);
                //下面的wait，empty函数没有锁的保护，感觉不合适。
//                s_taskVar.wait(lock, [](){
//                    return s_glThreadRunning && !s_tasks.empty();
//                });
                std::lock_guard guard(s_tasksLock);
                if(s_tasks.empty())//wait失败，可能是虚假唤醒
                {
                    continue;
                }
                auto func = s_tasks.front();
                s_tasks.pop();
                func();
            }
        }
        else
        {
            if(s_engine != nullptr)
            {
                delete s_engine;
                s_engine = nullptr;
            }
        }
    });
}
void stopService(){
    s_glThreadRunning = false;
    if(s_glThread.joinable())
    {
        s_glThread.join();
    }
}



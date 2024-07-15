//
// Created by lishi on 7/12/2024.
//

#include "NativeEngine.h"
#include <android/log.h>
#include <GLES3/gl32.h>
#include <EGL/egl.h>
#include <vector>
#define LOG_TAG "LocalSocketServer"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
static const int Width = 1920;
static const int Height = 1536;
namespace saturnv {
    NativeEngine::NativeEngine(EGLContext unityContext, EGLConfig unityConfig) {

        mSharedConfig = unityConfig;
        mSharedContext = unityContext;

        mEglDisplay = EGL_NO_DISPLAY;
        mEglSurface = EGL_NO_SURFACE;
        mEglContext = EGL_NO_CONTEXT;
        mEglConfig = nullptr;

        mHasFocus = mIsVisible = mHasWindow = false;
        mHasGLObjects = false;
        mIsFirstFrame = true;

        mSurfWidth = mSurfHeight = 0;
    }

    bool NativeEngine::InitDisplay() {
        if (mEglDisplay != EGL_NO_DISPLAY) {
            return true;
        }

        mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (EGL_FALSE == eglInitialize(mEglDisplay, nullptr, 0)) {
            LOGE("NativeEngine: failed to init display, error %d", eglGetError());
            return false;
        }
        return true;
    }

    bool NativeEngine::CreateTexture(int textureNum) {
        LOGI("%s:%d : CreateTexture num = %d!", __FILE_NAME__, __LINE__, textureNum);
        mTextureNum = textureNum;
        mTextures = new GLuint[textureNum];
        glGenTextures(textureNum,mTextures);
        auto e =glGetError();
        if(e != 0){
            LOGE("%s:%d : error = %x: glGenTexture failed!", __FILE_NAME__, __LINE__, e);
            return false;
        }
        LOGI("%s:%d : CreateTexture num = %d! ids = %x, %x, %x, %x", __FILE_NAME__, __LINE__, textureNum, mTextures[0], mTextures[1], mTextures[2], mTextures[3]);
        std::vector<char> pixels(Width * Height * 4);
        memset(pixels.data(),111,pixels.size() );
        for(int i = 0; i <textureNum;i++ )
        {
            auto* bytes = pixels.data();
            if(!UpdateTexture(mTextures[i],bytes,pixels.size())){
                continue;
            }
        }
        mCreateTextureSuccess = true;
        return true;
    }

    NativeEngine::~NativeEngine() {
        glBindTexture(GL_TEXTURE_2D, 0);
        if(mTextures != nullptr)
        {
            glDeleteTextures(mTextureNum, mTextures);
            delete[] mTextures;
            mTextures = nullptr;
        }
    }

    int NativeEngine::GetTextureId(int id) {
        LOGI("NativeEngine::GetTextureId, id = %d, m = %llx, num = %d", id, mTextures, mTextureNum);
        if(mTextures != nullptr && id < mTextureNum){
            return  mTextures[id];
        }
        return 0;
    }

    bool NativeEngine::InitOpenGL() {
        if(!InitDisplay())
        {
            return false;
        }
        int major, minor;
        if(!eglInitialize(mEglDisplay,&major,&minor  )){
            LOGE("%s:%d : error = %d: eglInitialize failed!", __FILE_NAME__, __LINE__, eglGetError());
            return false;
        }
        int attri[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
        };
        mEglContext = eglCreateContext(mEglDisplay,mSharedConfig, mSharedContext, attri);
        if(mEglContext == nullptr){
            LOGE("%s:%d : error = %d: eglInitialize failed!", __FILE_NAME__, __LINE__, eglGetError());
            return false;
        }
        int surfaceAttribList[] = {EGL_WIDTH, 1920, EGL_HEIGHT, 1536, EGL_NONE};
        mEglSurface = eglCreatePbufferSurface(mEglDisplay,mSharedConfig, surfaceAttribList);
        if(mEglSurface == nullptr){
            LOGE("%s:%d : error = %d: eglCreatePbufferSurface failed!", __FILE_NAME__, __LINE__, eglGetError());
            return false;
        }
        if(!eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)){
            LOGE("%s:%d : error = %d: eglCreatePbufferSurface failed!", __FILE_NAME__, __LINE__, eglGetError());
            return false;
        }
        glFlush();
        return true;
    }

    bool NativeEngine::UpdateTexture(int id, const char *data, int dataLength) {
        glBindTexture(GL_TEXTURE_2D, id);
        auto e =glGetError();
        if(e != 0){
            LOGE("%s:%d : error = %x: glBindTexture failed!", __FILE_NAME__, __LINE__, e);
            return  false;
        }
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0, GL_RGBA,Width, Height,0,GL_RGBA,GL_UNSIGNED_BYTE, data);
        e =glGetError();
        if(e != 0){
            LOGE("%s:%d : error = %d: glTexImage2D failed!", __FILE_NAME__, __LINE__, e);
            return false;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glFlush();
        return true;
    }

    bool NativeEngine::Running_AnyThread() {
        return mCreateTextureSuccess;
    }
} // saturnv
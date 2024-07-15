//
// Created by lishi on 7/12/2024.
//

#ifndef TESTANDROIDCPP_NATIVEENGINE_H
#define TESTANDROIDCPP_NATIVEENGINE_H
#include <EGL/egl.h>
namespace saturnv {
    //OpenGL引擎，处于独立线程
    class NativeEngine {
    public:
        //接收UnityContext
        NativeEngine(EGLContext unityContext, EGLConfig unityConfig);
        ~NativeEngine();
        bool InitOpenGL();

        bool CreateTexture(int textureNum);
        int GetTextureId(int id);
        bool UpdateTexture(int id, const char* data, int dataLength);
        //是否启动opengl成功，可以继续了？
        bool Running_AnyThread();
    private:
        bool InitDisplay();
        EGLDisplay mEglDisplay;
        EGLSurface mEglSurface;
        EGLContext mEglContext;
        EGLConfig mEglConfig;

        bool mHasFocus, mIsVisible, mHasWindow;
        bool mHasGLObjects;
        bool mIsFirstFrame;

        int mSurfWidth, mSurfHeight;

        EGLContext mSharedContext;
        EGLConfig mSharedConfig;

        //gluint，在egl里面，和gl里面定义不一致。
        unsigned int* mTextures;
        int mTextureNum;

        bool mCreateTextureSuccess = false;
    };

} // saturnv

#endif //TESTANDROIDCPP_NATIVEENGINE_H

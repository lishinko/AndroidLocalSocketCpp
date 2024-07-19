//
// Created by lishi on 7/12/2024.
//

#ifndef TESTANDROIDCPP_GLTEXTUREINTERFACE_H
#define TESTANDROIDCPP_GLTEXTUREINTERFACE_H

#include <GLES3/gl32.h>
#ifdef __cplusplus
extern "C"{
#endif

#pragma region C语言任务分配工具
#pragma pack(1)
    typedef struct _TaskData{
        void* data;
        int32_t length;
    }TaskData;
#pragma pack()
    //处理任务。0表示处理成功。
    //不管处理结果如何，用户都必须保证数据在DisposeTaskData回调之前有效。
    //如果返回值 ！= 0，则任务处理函数会打印日志.
    typedef int32_t (*HandleTask)(TaskData data);
    //销毁TaskData。用户自己销毁并保证销毁之后的数据不再使用
    typedef void (*DisposeTaskData)(TaskData data);
    //将任务添加到opengl线程.java代码应该封装该函数并调用。
    int32_t enqueueTask(HandleTask* handleCallback,DisposeTaskData* disposeCallback, TaskData data);
    void enqueueRenderTask(void* data, int length);
#pragma endregion

    //测试线程，正常代码不要调用。Unity调用，用来证明textureid被刷新了
    void startTestThread();
    void stopTestThread();
    //opengl任务线程是否启动成功？（成功 = opengl context已经成功 + 4个贴图创建好了）
    bool serviceRunning();
    //获得4个贴图的textureId，**必须在serviceRunning返回true之后调用**
    uint32_t getTexture(int32_t id);
    //开启opengl线程.重复调用不会重复开启
    void startService();
    //关闭opengl线程。重复关闭不会导致问题。
    void stopService();
#ifdef __cplusplus
};
#endif
#endif //TESTANDROIDCPP_GLTEXTUREINTERFACE_H

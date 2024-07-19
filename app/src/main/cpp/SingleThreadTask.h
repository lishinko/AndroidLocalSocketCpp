//
// Created by lishi on 7/12/2024.
//

#ifndef TESTANDROIDCPP_SINGLETHREADTASK_H
#define TESTANDROIDCPP_SINGLETHREADTASK_H
#include <functional>
#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <jni.h>
namespace saturnv {

    class SingleThreadTask {
    public:
        SingleThreadTask(bool waitForAllTasksWhenReleasing);
        ~SingleThreadTask();
        void Enqueue(std::function<void()> func);
        //析构函数执行之前，等待所有任务执行完毕.注意，默认析构函数*不会*等待所有任务执行完成！！！！
        void EnsureAttachJvm(JavaVM* jvm);
        JNIEnv* GetThreadEnv(){
            return mEnv;
        }
        void DetachJvm();
    private:
//        void WaitForAllTasksAndRelease();
        void StartThread();
        std::queue<std::function<void()>> mTasks;
        std::mutex mTasksLock;

        std::thread mGlThread;
        //线程正在运行
        std::atomic_bool mGlThreadRunning;
        std::atomic_bool mGlThreadClosing;
        std::condition_variable mTaskVar;
        std::mutex mTaskVarLock;

        //从s_tasks里面取出的过程需要加锁。通过缓存的方式减少锁
        std::vector<std::function<void()>> mTempFuncs;
        //退出之前，是否要等待积压的任务完成？
        bool mWaitForAllTasksWhenReleasing;

        //java要求所有线程都附加jvm才行
        JavaVM* mJvm = nullptr;
        JNIEnv* mEnv = nullptr;
    };

} // saturnv

#endif //TESTANDROIDCPP_SINGLETHREADTASK_H

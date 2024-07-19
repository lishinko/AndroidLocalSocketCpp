//
// Created by lishi on 7/12/2024.
//

#include "SingleThreadTask.h"
#include <android/log.h>
#include <vector>

#define LOG_TAG "LocalSocketServer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace saturnv {
    SingleThreadTask::~SingleThreadTask() {
        mGlThreadClosing = true;
        Enqueue([this](){
            DetachJvm();
            mGlThreadRunning = false;
        });
//        WaitForAllTasksAndRelease();
//立即关闭执行，此时渲染线程状态：
//1,wait_for：到期自动停止
//2,正在执行其他任务：执行完毕自动停止
//3，在mGlThreadRunning = false之后,enqueue不再接收新任务，故上山1，2状态一定可以停止
        if(mGlThread.joinable())
        {
            mGlThread.join();
        }
    }

    void SingleThreadTask::Enqueue(std::function<void()> func) {
        if(mGlThreadRunning == false || mGlThreadClosing)
        {
            LOGE("SingleThreadTask::Enqueue, task is closing, the func is rejected! thread = %d", std::this_thread::get_id());
            return;
        }
        mTasksLock.lock();
        mTasks.push(func);
        mTasksLock.unlock();
        mTaskVar.notify_one();
    }

    void SingleThreadTask::StartThread() {
        LOGI("SingleThreadTask::StartThread! thread = %d", std::this_thread::get_id());
        mGlThreadRunning = true;
        mGlThread = std::thread([this](){
            LOGI("SingleThreadTask::StartThread!, thread = %d", std::this_thread::get_id());
            std::unique_lock<std::mutex> lock(mTaskVarLock);
            while (mGlThreadRunning)
            {
                std::chrono::milliseconds waitDur(300);
                mTaskVar.wait_for(lock, waitDur);
                //下面的wait，empty函数没有锁的保护，感觉不合适。
//                mTaskVar.wait(lock, [](){
//                    return mGlThreadRunning && !mTasks.empty();
//                });
//                std::lock_guard guard(mTasksLock);
                mTempFuncs.clear();
                mTasksLock.lock();
                while (!mTasks.empty())//wait失败，可能是虚假唤醒
                {
                    auto func = mTasks.front();
                    mTasks.pop();
                    mTempFuncs.push_back(func);
                }
                mTasksLock.unlock();
                for(auto& f : mTempFuncs){
//                    LOGI("SingleThreadTask::StartThread executing funcs!");
                    f();
                }
                mTempFuncs.clear();
            }
        });
    }

    SingleThreadTask::SingleThreadTask(bool waitForAllTasksWhenReleasing)
    :mWaitForAllTasksWhenReleasing(waitForAllTasksWhenReleasing)
    {
        mGlThreadRunning = false;
        mGlThreadClosing = false;
        mJvm = nullptr;
        mEnv = nullptr;
        StartThread();
    }

//    void SingleThreadTask::WaitForAllTasksAndRelease() {
//        mGlThreadClosing = true;
//        Enqueue([this](){
//            //正常逻辑会让它执行完所有任务的，我们只需要保证任务执行完毕之前,不要关闭mGlThreadRunning即可
//            mGlThreadRunning = false;//在这里关闭，可以避免循环进入wait_for函数而卡死
//        });
//        if(mGlThread.joinable())
//        {
//            mGlThread.join();
//        }
//    }
    void SingleThreadTask::EnsureAttachJvm(JavaVM* jvm)
    {
        if(mEnv != nullptr && mJvm != nullptr)
        {
            LOGE("jvm already attached!");
            return;
        }
        mJvm = jvm;
        if(mJvm != nullptr)
        {
            JNIEnv* currentThreadEnv = nullptr;
            mJvm->AttachCurrentThread( &currentThreadEnv, nullptr);
            mEnv = currentThreadEnv;
        }
        LOGI(" jvm = %llx, newEnv = %llx",  mJvm, mEnv);
    }
    void SingleThreadTask::DetachJvm()
    {
        if(mEnv != nullptr && mJvm != nullptr)
        {
            mJvm->DetachCurrentThread();
            mEnv = nullptr;
            mJvm = nullptr;
        }
    }
} // saturnv
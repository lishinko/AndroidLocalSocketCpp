//
// Created by lishi on 7/12/2024.
//

#include "SingleThreadTask.h"
#include <vector>

#define LOG_TAG "LocalSocketServer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace saturnv {
    SingleThreadTask::~SingleThreadTask() {

        s_glThreadRunning = false;
        if(s_glThread.joinable())
        {
            s_glThread.join();
        }
    }

    void SingleThreadTask::Enqueue(std::function<void()> func) {
        if(!s_glThread.joinable())
        {
            StartThread();
        }
        s_tasksLock.lock();
        s_tasks.push(func);
        s_tasksLock.unlock();
        s_taskVar.notify_one();
    }

    void SingleThreadTask::StartThread() {
        s_glThread = std::thread([this](){
            std::unique_lock lock(s_taskVarLock);
            while (s_glThreadRunning)
            {
                std::chrono::milliseconds waitDur(1000);
                s_taskVar.wait_for(lock,waitDur);
                //下面的wait，empty函数没有锁的保护，感觉不合适。
//                s_taskVar.wait(lock, [](){
//                    return s_glThreadRunning && !s_tasks.empty();
//                });
//                std::lock_guard guard(s_tasksLock);
                s_tasksLock.lock();
                mTempFuncs.clear();
                while (!s_tasks.empty())//wait失败，可能是虚假唤醒
                {
                    auto func = s_tasks.front();
                    s_tasks.pop();
                    mTempFuncs.push_back(func);
                }
                s_tasksLock.unlock();
                for(auto& f : mTempFuncs){
                    f();
                }
                mTempFuncs.clear();
            }
        });
    }
} // saturnv
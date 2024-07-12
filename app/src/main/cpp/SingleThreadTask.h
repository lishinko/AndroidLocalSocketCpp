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
namespace saturnv {

    class SingleThreadTask {
    public:
        ~SingleThreadTask();
        void Enqueue(std::function<void()> func);
    private:
        void StartThread();
        std::queue<std::function<void()>> s_tasks;
        std::mutex s_tasksLock;

        std::thread s_glThread;
        std::atomic_bool s_glThreadRunning = false;
        std::condition_variable s_taskVar;
        std::mutex s_taskVarLock;

        //从s_tasks里面取出的过程需要加锁。通过缓存的方式减少锁
        std::vector<std::function<void()>> mTempFuncs;
    };

} // saturnv

#endif //TESTANDROIDCPP_SINGLETHREADTASK_H

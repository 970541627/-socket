//
// Created by 97054 on 2020/12/4.
//

#ifndef THREADPOOLBYONLY_ONLYTHREADPOOL_H
#define THREADPOOLBYONLY_ONLYTHREADPOOL_H


#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <thread>
#include <future>


namespace std {


    class ThreadPool {

    private:
        //最大线程数
        const int MAX_THREAD_SIZE = 8;
        //最大任务数
        const int MAX_TASK_SIZE = 20;
        //任务太多时，额外增加临时的线程
        const int MAX_ADD_THREAD = 3;

        using Task = std::function<void()>;

        //任务队列
        using task_t = std::deque<Task>;
        task_t m_task;

        //线程队列
        std::vector<std::thread *> threads;

        //添加互斥锁
        mutex mutex;

        //添加条件变量（条件变量用于是阻塞当前线程，直到被唤醒）,
        condition_variable lockThread;

        //是否处于运行状态
        atomic<bool> isStart;

        //任务数量
        atomic<int> num;

//        处理额外任务时候
        deque<Task> *saveTas;

        //额外任务数
        int saveTasCount = 4;

        //正在运行的线程数量
        int threadQu=0;

    public:
        ThreadPool()
                : isStart(true),
                  num(0) {
            start();
        };

        ThreadPool(int INIT_THREAD_SIZE, int INIT_TASK_SIZE, int INIT_ADD_THREAD_SIZE)
                : isStart(true),
                  num(0),
                  MAX_THREAD_SIZE(INIT_THREAD_SIZE),
                  MAX_TASK_SIZE(INIT_TASK_SIZE),
                  MAX_ADD_THREAD(INIT_ADD_THREAD_SIZE) {

        };

        ~ThreadPool() {
            stop();
        };

        /**
         * 线程销毁，只是先唤醒所有的线程，然后用join全部处于等待状态
         */


        //匿名创建线程并添加入vector容器;
        void start() {
            threads.reserve(MAX_THREAD_SIZE + 5);
            for (int thread = 0; thread < MAX_THREAD_SIZE; thread++) {
                threads.emplace_back(new std::thread(std::bind(&ThreadPool::threadLoop, this)));
            }
        }


                      /**
                       * 多线程的添加任务队列板块，template是进行反省编程的关键字,两个&&是右值引用
                       * @tparam Type 获取函数对象，类成员函数对象等等
                       * @tparam Args 可变参类型，获取各个值
                       */
        template<class Type, class...Args>
        auto addTask(Type &&type, Args &&...args) -> std::shared_future<decltype(type(args...))> {
            std::lock_guard<std::mutex> lock(mutex);
            using RetType = decltype(type(args...));

            //auto是C++11的特性，可以自动推导右边方法的返回类型;采用了异步的方式，即调用了packaged_task函数；
            auto task = std::make_shared<std::packaged_task<RetType()> >(
                    std::bind(std::forward<Type>(type), std::forward<Args>(args)...)
            );
            //获取异步结果，用future类型来接收
            std::shared_future<RetType> future = task->get_future();
            if (m_task.size() <= MAX_TASK_SIZE) {
                {
                    m_task.emplace_front([task]() {
                        (*task)();
                    });
                }
                num++;
                lockThread.notify_one();
                return future;
            }else {
                //判断当前的任务队列数量是不是大于规定值
                if (saveTas->size() != saveTasCount) {

                    //C++可以把函数作为参数，所以saveTas接收到的新参数是task函数；
                    saveTas->emplace_back([task]() {
                        (*task)();
                    });

                    saveTasCount++;
                    addThread();
                    return future;
                }
            }
        };

        //获取当前任务数量
        int getTaskQuality() {
            return num;
        }
        //获取当前运行的线程数量
        int getThreadQuality() {
            return threadQu;
        }

    private:

        //当任务数大于MAX_TASK_SIZE，额外创建临时线程
        void addThread() {
            auto g = std::bind(&ThreadPool::dealTask, this);
            std::thread *th = new thread(g);
            th->join();
        }


        void dealTask() {
            Task getTask = saveTas->front();
            getT();
            saveTas->pop_back();
            saveTasCount--;
        }

        void threadLoop() {
            while (isStart) {
                //std::cout<<"this is ="<<std::this_thread::get_id<<"\n";
                Task tas = getT();
                if (tas != NULL) {
                  //  lock_guard<std::mutex> lock(mutex);
                    tas();
                    threadQu--;
                }


                num--;
            }
        };

        void stop() {
            isStart = false;
            //unique_lock<std::mutex> lock{mutex};
            lockThread.notify_all();
            for (thread *th:threads) {
                if (th->joinable())
                    th->join();
                delete th;
            }
            threads.clear();
        };


        Task getT() {
            unique_lock<std::mutex> lock{mutex};
            while (m_task.empty() && isStart) {
                lockThread.wait(lock, [this] {
                    return !isStart.load() || !m_task.empty();
                });
            }
            threadQu++;
            Task task = m_task.front();
            m_task.pop_front();
            return task;
        }

    };

}
#endif //THREADPOOLBYONLY_ONLYTHREADPOOL_H
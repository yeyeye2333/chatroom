#ifndef chat_Server_Threadpool
#define chat_Server_Threadpool
#include"chat_Threadpool_Task.hpp"
#include<functional>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<list>
#include<unistd.h>
#ifndef Threads_max
#define Threads_max 12
#endif
using std::thread;
class Threadpool{
public:
    Threadpool(unsigned int val=6,unsigned int val2=-1)
    :cur_threads(val>Threads_max?Threads_max:val),task(val2),max_task(val2)
    {   
        for(int c=0;c<cur_threads;c++)
        {   
            threads.emplace_back(&Threadpool::working,this);
        }
    }
    template<typename T,typename...A>bool addtask(T&&atask,A&&...argc)
    {
        auto result=task.push(std::bind(std::forward<T>(atask),std::forward<A>(argc)...));
        if(result==0)
        {
            pool_cond.notify_all();
            return 0;
        }
        pool_cond.notify_all();
        return 1;
    }
    bool addthread();
    void terminal()
    {
        term=true;
        pool_cond.notify_all();
        for(auto &a:threads)
        {
            if(a.joinable())a.join();
        }
    }
    ~Threadpool()
    {
        terminal();
    }
    unsigned int thread_num(){return cur_threads;}
    unsigned int task_num(){return task.size();}
private:
    Task<std::function<void()>> task;
    unsigned int max_task;
    std::list<thread> threads;
    unsigned int max_threads=Threads_max;
    unsigned int cur_threads;
    bool term=false;
    std::mutex pool_mtx;
    std::condition_variable pool_cond;
private:
    void working();
};

void Threadpool::working()
{
    std::unique_lock<std::mutex> pool_ulock(pool_mtx,std::defer_lock);
    while(term==false||task.size()!=0)
    {
        auto atask=task.pop();
        if(atask!=nullptr)
        {
            atask();
        }
        else
        {
            pool_ulock.lock();
            pool_cond.wait(pool_ulock);
            pool_ulock.unlock();
        }
    }
    return ;
}
bool Threadpool::addthread()
{
    if(cur_threads==Threads_max)
        return 0;
    else
    {
        ++cur_threads;
        threads.emplace_back(&Threadpool::working,this);
        return 1;
    }
}

#endif
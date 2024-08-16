#ifndef chat_Server_Threadpool_Task
#define chat_Server_Threadpool_Task
#include<memory>
#include<mutex>
#include<queue>
using std::unique_ptr;
template<typename T>
class TaskQueue {
public:
    explicit TaskQueue(unsigned int val):capacity(val){}
    virtual bool push(T) = 0;
    virtual T pop() = 0;
    virtual ~TaskQueue() {};
protected:
    unsigned int capacity;
};
template<typename T>
class Task:public TaskQueue<T>{
public:
    explicit Task(unsigned int val=-1):TaskQueue<T>(val){}
    bool push(T val)
    {
        std::unique_lock<std::mutex> ulock(mtx);
        if(cur<this->capacity)
        {
            que.push(val);
            cur++;
            ulock.unlock();
            return 1;
        }
        else
        {
            ulock.unlock();
            return 0;
        }
    }
    T pop()
    {
        std::unique_lock<std::mutex> ulock(mtx);
        if(cur==0)
        {
            ulock.unlock();
            return nullptr;
        }
        else
        {
            cur--;
            auto ret=que.front();
            que.pop();
            ulock.unlock();
            return ret;
        }
    }
    unsigned int size()
    {
        return cur;
    }
    ~Task(){};
private:
    std::queue<T> que;
    unsigned int cur=0;
    std::mutex mtx;
};

#endif
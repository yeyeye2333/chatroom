#ifndef chat_Server
#define chat_Server
#include"../chat_Timer.hpp"
#include"../chat_Threadpool.hpp"
#include"../chat_Socket.hpp"
#include"chat_Server_Reactor.hpp"
#include<time.h>
char cur_time[20]={0};//%Y-%m-%d %H:%M:%S

class Server{
public:
    Server(int _num=4):num(_num),threads(_num){}
    void start();
private:
    int num;
    Threadpool threads;
};
void handler(Reactor& one)
{
    while (true)
    {
        // auto start = std::chrono::high_resolution_clock::now();
        
        auto pollque=one.poll();
        for(auto tmp:pollque)
        {
            // auto start2 = std::chrono::high_resolution_clock::now();
            tmp->deal();
            // auto end2 = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double> duration2 = end2 - start2;
            // std::cout << "deal运行时间: " << duration2.count() << " 秒" << std::endl;
        }
        // 结束计时
        // auto end = std::chrono::high_resolution_clock::now();

        // 计算时间差
        // std::chrono::duration<double> duration = end - start;
        // std::cout << "poll+deal运行时间: " << duration.count() << " 秒" << std::endl;
    }
}
void Server::start()
{
    Timer timer(2,1);
    timer.init();
    Reactor reactor[num];
    for(int c=0;c<num;c++)    
    {
        threads.addtask(handler,std::ref(reactor[c]));
    }
    for(auto &tmp:reactor)
    {
        for(auto &tmp2:reactor)
        {
            tmp.addreac(&tmp2);
        }
    }
    timer.start();
    Socket_listen listen;
    listen._bind();
    if(listen._listen()==0)exit(EXIT_FAILURE);
    int fd;
    while(true)
    {
        if((fd=listen._accept())!=-1)
        {
            std::cerr<<"add="<<fd<<"\n";
            reactor[fd%num].addfd(fd);
        }
        else if(timer.timer_flag>0)
        {
            timer.timer_flag--;
            auto tmp=time(nullptr);
            strftime(cur_time,20,"%Y-%m-%d %H:%M:%S",localtime(&tmp));
            if(timer.timer_flag>=30)
            {
                timer.timer_flag-=30;
                for(auto&tmp:reactor)
                {
                    auto tmp2=tmp.heart_check();
                    if(tmp2>0)std::cerr<<tmp2<<"个客户端无心跳";
                }
            }
        }
    }
}

#endif
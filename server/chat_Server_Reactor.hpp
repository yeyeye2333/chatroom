#ifndef chat_Server_Reactor
#define chat_Server_Reactor
#include<unistd.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<vector>
#include<memory>
#include<map>
#include<mutex>
#include<shared_mutex>
#include"chat_Server_Save.hpp"
#include"../chatroom.pb.h"
#include<mutex>
#include<iostream>
#include <sys/sendfile.h>
#include<sys/stat.h>
#include<fcntl.h>
#define maxlen 65536
#define maxhead_len 10


using chatroom::Type;
class Reactor;
enum Reactor_type{nocheck,checked};
class Clannel{
public:
    Clannel(Reactor*_reac,int _fd,uint32_t _rev)
        :reactor(_reac),fd(_fd),revent(_rev),ulock(mtx,std::defer_lock){}
    virtual void deal()=0;
    virtual ~Clannel(){}
    virtual void _send(Type type,bool is,std::vector<string> v0={},std::vector<string> v1={},
            std::vector<string> v2={},std::vector<string> v3={},int id=0,int id2=0){};
    void set_event(uint32_t=0);
protected:
    Reactor*reactor;
    int fd;
    uint32_t revent;
    bool recv_cache();
    void reset();
    virtual void quit()=0;
    chatroom::Head head;
    std::string cache;
    std::mutex mtx;
    std::unique_lock<std::mutex> ulock;
};
class Clannel_checked:public Clannel{
    friend class Clannel_nocheck;
public:
    Clannel_checked(Reactor*_reac,int _fd,uint32_t _rev,int _uid)
        :Clannel(_reac,_fd,_rev),uid(_uid),uid_ulock(mtx,std::defer_lock)
    {
        uid_ulock.lock();
        auto tmp=uid_map.insert({uid,fd});
        std::cerr<<"uid插入"<<tmp.second<<std::endl;
        uid_ulock.unlock();
        offset.reset(new off_t(0));
    }
    ~Clannel_checked(){}
    void deal();
    void _send(Type type,bool is,std::vector<string> v0={},std::vector<string> v1={},
            std::vector<string> v2={},std::vector<string> v3={},int id=0,int id2=0);
    std::mutex mtx;
private:
    void realsend(const string& sendstr,bool is,Type type);
    void quit();
    int uid;
    static std::map<int,int> uid_map;
    static std::mutex uid_mtx;
    std::unique_lock<std::mutex> uid_ulock;
    long need_recv=0;
    long need_send=0;
    unique_ptr<off_t> offset;
    std::vector<std::tuple<Type ,bool ,std::vector<string> ,std::vector<string> ,
                std::vector<string> ,std::vector<string> ,int ,int >>wait_que;
    std::mutex que_mtx;
};
std::map<int,int> Clannel_checked::uid_map;
std::mutex Clannel_checked::uid_mtx;
class Clannel_nocheck:public Clannel{
public:
    Clannel_nocheck(Reactor*_reac,int _fd,uint32_t _rev)
        :Clannel(_reac,_fd,_rev){}
    ~Clannel_nocheck(){}
    void deal();
    void _send(Type type,bool is,std::vector<string> v0,std::vector<string> v1,
            std::vector<string> v2,std::vector<string> v3,int id,int id2){}
private:
    void _send(Type type,bool is,int id=0,std::vector<string> v={});
    void quit();
};
struct fd_key{
    fd_key()=default;
    fd_key(Clannel*ptr,bool is):clannel(ptr),heart(is){}
    std::shared_ptr<Clannel> clannel;
    bool heart;
};

class Reactor{
    friend Clannel_checked;
    friend Clannel_nocheck;
public:
    Reactor():ulock(mtx,std::defer_lock),slock(mtx,std::defer_lock){efd=epoll_create1(EPOLL_CLOEXEC);}
    ~Reactor(){close(efd);}
    void addreac(Reactor*ptr)
    {
        reactors.push_back(ptr);
        reac_num++;
    }
    template<typename...T>
    bool addfd(int fd,Reactor_type type=Reactor_type::nocheck,int id=0)
    {
        if(efd==-1)return 0;
        epoll_event ev;
        if (type==nocheck)
        {
            ev.data.fd=fd;
            ev.events=EPOLLIN|EPOLLRDHUP;
            if(epoll_ctl(efd,EPOLL_CTL_ADD,fd,&ev)==-1)return 0;
            ulock.lock();
            fd_map.emplace(fd,fd_key(new Clannel_nocheck(this,fd,0),1));
            cur++;
            ulock.unlock();
        }
        else if(type==checked)
        {
            ev.data.fd=fd;
            ev.events=EPOLLIN|EPOLLRDHUP;
            if(epoll_ctl(efd,EPOLL_CTL_ADD,fd,&ev)==-1)return 0;
            ulock.lock();
            fd_map.emplace(fd,fd_key(new Clannel_checked(this,fd,0,id),1));
            cur++;
            ulock.unlock();
        }
        return 1;
    }
    bool reducefd(int fd)
    {
        if(efd==-1)return 0;
        ulock.lock();
        fd_map.erase(fd);
        if(epoll_ctl(efd,EPOLL_CTL_DEL,fd,nullptr)==0)
        {
            cur--;
            ulock.unlock();
            return 1;
        }
        else
        {
            ulock.unlock();
            return 0;
        }
    }
    template<typename...T>
    bool modfd(int fd,Reactor_type type,int id)
    {
        reducefd(fd);
        return addfd(fd,type,id);
    }
    std::vector<std::shared_ptr<Clannel>> poll(int maxevents=10,int timeout=-1)
    {
        epoll_event ev_list[maxevents];
        int ev_num;
        std::vector<std::shared_ptr<Clannel>> tmp;
        // auto start2 = std::chrono::high_resolution_clock::now();
        if((ev_num=epoll_wait(efd,ev_list,maxevents,timeout))>0)
        {
            // auto end2 = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<double> duration2 = end2 - start2;
            // std::cout << "epoll_wait运行时间: " << duration2.count() << " 秒" << std::endl;
            slock.lock();
            for(int c=0;c<ev_num;c++)
            {
                heart_beat(ev_list[c].data.fd);
                fd_map[ev_list[c].data.fd].clannel->set_event(ev_list[c].events);
                tmp.push_back(fd_map[ev_list[c].data.fd].clannel);
            }
            slock.unlock();
        }
        return tmp;
    }
    int heart_check()
    {
        int res=0;
        std::vector<int>erases;
        ulock.lock();
        for(auto&tmp:fd_map)
        {
            if(tmp.second.heart==1)tmp.second.heart=0;
            else
            {
                res++;
                erases.push_back(tmp.first);
            }
        }
        for(auto &tmp:erases){
            fd_map.erase(tmp);
        }
        ulock.unlock();
        return res;
    }
    // 现在无锁
    void heart_beat(int fd)
    {
        // ulock.lock();
        try{
            fd_map.at(fd).heart=1;
        }catch(std::out_of_range){}
        // ulock.unlock();
        return ;
    }
    
    // 开始发送大量无标签数据，不允许发送其他数据,同时关闭可读，防止心跳包导致轮巡
    bool add_epollout(int fd){
        if(efd==-1)return false;
        epoll_event ev;
        ev.data.fd=fd;
        ev.events=EPOLLRDHUP|EPOLLOUT;
        if(epoll_ctl(efd,EPOLL_CTL_MOD,fd,&ev)==-1)return false;
        return true;
    }
    // 结束发送大量无标签数据，可允许发送其他数据
    bool rm_epollout(int fd){
        if(efd==-1)return false;
        epoll_event ev;
        ev.data.fd=fd;
        ev.events=EPOLLIN|EPOLLRDHUP;
        if(epoll_ctl(efd,EPOLL_CTL_MOD,fd,&ev)==-1)return false;
        return true;
    }
private:
    int efd;
    int cur;
    std::map<int,fd_key> fd_map;
    std::shared_mutex mtx;
    std::unique_lock<std::shared_mutex> ulock;
    std::shared_lock<std::shared_mutex> slock;
    Save DB;
    std::vector<Reactor*> reactors;// send可能竞争，待改（给realsend加锁） 客户端接受文件可能收到混杂消息，待改（存realsend要发的消息）
    int reac_num=0;
};



void Clannel::set_event(uint32_t event)
{
    revent=event;
}
bool Clannel::recv_cache()
{
    if(head.has_type()==0)
    {
        char head_len=0;
        if(recv(fd,&head_len,sizeof(head_len),MSG_DONTWAIT)!=sizeof(head_len)||head_len>maxhead_len||head_len<0)//非法客户端
        {
            quit();
            return 0;
        }
        char headstr[head_len];
        if(recv(fd,headstr,head_len,MSG_DONTWAIT)<head_len)//非法客户端
        {
            quit();
            return 0;
        }
        head.ParseFromArray(headstr,head_len);
        if(head.len()>maxlen||head.len()<0)
        {
            quit();
            return 0;
        }
        if(head.len()==0)return 1;
        char tmp[head.len()];
        int tmp_len;
        if((tmp_len=recv(fd,tmp,head.len(),MSG_DONTWAIT))<head.len())
        {
            if(tmp_len<0)return 0;
            cache.assign(tmp,tmp_len);
            return 0;
        }
        else
        {
            cache.assign(tmp,tmp_len);
            return 1;
        }
    }else if((head.len()-cache.size())>0){
        char tmp[head.len()-cache.size()];
        int tmp_len;
        if((tmp_len=recv(fd,tmp,head.len()-cache.size(),MSG_DONTWAIT))<(head.len()-cache.size()))
        {
            if(tmp_len<0)return 0;
            cache.append(tmp,tmp_len);
            return 0;
        }
        else
        {
            cache.append(tmp,tmp_len);
            return 1;
        }
    }else{
        return 1;
    }
}
void Clannel::reset()
{
    head.clear_len();
    head.clear_type();
    head.clear_is();
    cache="";
}

void set_Head(string*s_ptr,bool is,Type type,int len)
{
    chatroom::Head _Head;
    _Head.set_is(is);
    _Head.set_len(len);
    _Head.set_type(type);
    _Head.SerializeToString(s_ptr);
}
void set_File(string*s_ptr,const std::vector<int> &obj={},const std::vector<string>& name={},const std::vector<string>& context={},const std::vector<string>& date={},int gid=0)
{
    chatroom::File _File;
    for(auto &tmp:obj)_File.add_obj(tmp);
    for(auto &tmp:name)_File.add_name(tmp);
    for(auto &tmp:context)_File.add_len(std::stol(tmp));
    for(auto &tmp:date)_File.add_date(tmp);
    if(gid!=0)_File.set_gid(gid);
    _File.SerializePartialToString(s_ptr);
}
void set_Message(string*s_ptr,const std::vector<int>&obj={},const std::vector<string>&context={},const std::vector<string>&date={},int gid=0)
{
    chatroom::Message _Message;
    for(auto &tmp:obj)_Message.add_obj(tmp);
    for(auto &tmp:context)_Message.add_context(tmp);
    for(auto &tmp:date)_Message.add_date(tmp);
    if(gid!=0)_Message.set_gid(gid);
    _Message.SerializePartialToString(s_ptr);
}
void set_IDs(string*s_ptr,const std::vector<int>&id={})
{
    chatroom::IDs _IDs;
    for(auto &tmp:id)_IDs.add_id(tmp);
    _IDs.SerializePartialToString(s_ptr);
}
void set_Signup_info(string*s_ptr,const int& uid,const string&name="",const string&password="")
{
    chatroom::Signup_info _Signup;
    _Signup.set_uid(uid);
    _Signup.set_name(name);
    _Signup.set_password(password);
    _Signup.SerializePartialToString(s_ptr);
}
void set_Login_info(string*s_ptr,const int&uid,const string&password="")
{
    chatroom::Login_info _Login;
    _Login.set_uid(uid);
    _Login.set_password(password);
    _Login.SerializePartialToString(s_ptr);
}
void set_Group_uid(string*s_ptr,const int&uid,const string&name="")
{
    chatroom::Group_uid _Group;
    _Group.set_uid(uid);
    _Group.set_name(name);
    _Group.SerializePartialToString(s_ptr);
}
void set_Strs(string*s_ptr,const std::vector<string>&str)
{
    chatroom::Strs _Str;
    for(auto&tmp:str)_Str.add_str(tmp);
    _Str.SerializePartialToString(s_ptr);
}


//Clannel_checked
bool rm_in_chatroom(string fname,int id1,int id2=0){
    if(id2=0){
        return remove(("/var/lib/chatroom_files/g"+std::to_string(id1)).c_str());
    }else{
        if (remove(("/var/lib/chatroom_files/u"+std::to_string(id1)+std::to_string(id2)+fname).c_str())==-1){
            return remove(("/var/lib/chatroom_files/u"+std::to_string(id2)+std::to_string(id1)+fname).c_str());
        }else{
            return true;
        }
    }
}
long append_in_chatroom(const void*tmp, size_t count,string fname,int id1,int id2=0){
    int fd=-1;
    if(id2==0){
        fd=open(("/var/lib/chatroom_files/g"+std::to_string(id1)+fname).c_str(),O_WRONLY|O_CREAT|O_APPEND,0664);
    }else{
        fd=open(("/var/lib/chatroom_files/u"+std::to_string(id1)+std::to_string(id2)+fname).c_str(),O_WRONLY|O_CREAT|O_APPEND,0664);
        if (fd<0)
        {
            open(("/var/lib/chatroom_files/u"+std::to_string(id2)+std::to_string(id1)+fname).c_str(),O_WRONLY|O_CREAT|O_APPEND,0664);
        }
    }
    int ret=0;
    if(fd>=0){
        ret=write(fd,tmp,count);
        close(fd);
    }
    if(ret<=0){
        std::cerr<<"368 "<<errno<<std::endl;
    }
    return ret;
}
void  Clannel_checked::realsend(const string&sendstr,bool is,Type type)
{
    string sendhead;
    if(is==0)std::cerr<<"mysql_err:"<<reactor->DB.mysql_err()<<"\n";
    set_Head(&sendhead,is,type,sendstr.size());
    char len=sendhead.size();
    send(fd,(string(&len,sizeof(len))+sendhead).c_str(),sizeof(len)+len,0);
    if(sendstr.size()>0)send(fd,sendstr.c_str(),sendstr.size(),0);
}
void Clannel_checked::_send(Type type,bool is,std::vector<string> v0,std::vector<string> v1,
                        std::vector<string> v2,std::vector<string> v3,int id,int id2)
{
    std::vector<int> tmp;
    string sendstr;
    std::unique_lock<std::mutex> tmp_lock(mtx,std::defer_lock);
    if(type==Type::u_f_history1||type==Type::g_f_history1){
        // 需外部锁定
    }else if(type==Type::notify_g_f||type==Type::notify_g_m||type==Type::notify_g_req||
        type==Type::notify_u_f||type==Type::notify_u_m||type==Type::notify_u_req){
        que_mtx.lock();
        if(!tmp_lock.try_lock()){
            // 已锁定，添加至缓冲区中等待发送
            wait_que.push_back( std::make_tuple(type, is ,v0, v1, v2, v3, id, id2));
            que_mtx.unlock();
            return;
        }
        que_mtx.unlock();
    }else{
        tmp_lock.lock();
    }
    switch (type)
    {
        case Type::u_search:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_Message(&sendstr,tmp,v1);
            realsend(sendstr,is,Type::u_search);
            break;
    
        case Type::u_request:
            realsend(sendstr,is,Type::u_request);
            break;
        
        case Type::u_listreq:
            set_Strs(&sendstr, v0);
            realsend(sendstr, is, Type::u_listreq);
            break;

        case Type::u_add:
            realsend(sendstr, is, Type::u_add);
            break;

        case Type::u_del:
            realsend(sendstr, is, Type::u_del);
            break;

        case Type::u_blok:
            realsend(sendstr, is, Type::u_blok);
            break;

        case Type::u_unblok:
            realsend(sendstr, is, Type::u_unblok);
            break;

        case Type::u_message:
            realsend(sendstr, is, Type::u_message);
            break;

        case Type::u_file:
            realsend(sendstr, is, Type::u_file);
            break;

        case Type::u_m_history:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_Message(&sendstr,tmp,v1,v2);
            realsend(sendstr, is, Type::u_m_history);
            break;

        case Type::u_f_history0:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_File(&sendstr,tmp,v1,{},v2);
            realsend(sendstr, is, Type::u_f_history0);
            break;

        case Type::u_f_history1:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_File(&sendstr,tmp,v1,v2,v3);
            realsend(sendstr, is, Type::u_f_history1);
            break;

        case Type::g_create:
            set_IDs(&sendstr,{id});
            realsend(sendstr, is, Type::g_create);
            break;

        case Type::g_disban:
            realsend(sendstr, is, Type::g_disban);
            break;

        case Type::g_request:
            realsend(sendstr, is, Type::g_request);
            break;

        case Type::g_listreq:
            set_Strs(&sendstr,v0);
            realsend(sendstr, is, Type::g_listreq);
            break;

        case Type::g_add:
            realsend(sendstr, is, Type::g_add);
            break;

        case Type::g_del:
            realsend(sendstr, is, Type::g_del);
            break;

        case Type::g_search:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_Message(&sendstr,tmp,v1);
            realsend(sendstr, is, Type::g_search);
            break;

        case Type::g_message:
            realsend(sendstr, is, Type::g_message);
            break;

        case Type::g_file:
            realsend(sendstr, is, Type::g_file);
            break;

        case Type::g_quit:
            realsend(sendstr, is, Type::g_quit);
            break;

        case Type::g_members:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_Message(&sendstr,tmp,v1,v2);
            realsend(sendstr, is, Type::g_members);
            break;

        case Type::g_addmanager:
            realsend(sendstr, is, Type::g_addmanager);
            break;

        case Type::g_delmanager:
            realsend(sendstr, is, Type::g_delmanager);
            break;

        case Type::g_m_history:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_Message(&sendstr,tmp,v1,v2);
            realsend(sendstr, is, Type::g_m_history);
            break;

        case Type::g_f_history0:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_File(&sendstr,tmp,v1,{},v2);
            realsend(sendstr, is, Type::g_f_history0);
            break;

        case Type::g_f_history1:
            for(auto &tmp2:v0)tmp.push_back(std::stoi(tmp2));
            set_File(&sendstr,tmp,v1,v2,v3);
            realsend(sendstr, is, Type::g_f_history1);
            break;

        case Type::notify_u_req:
            set_IDs(&sendstr,{id});
            realsend(sendstr, is, Type::notify_u_req);
            break;

        case Type::notify_u_m:
            set_Message(&sendstr,{id},v0);
            realsend(sendstr, is, Type::notify_u_m);
            break;

        case Type::notify_u_f:
            set_File(&sendstr,{id},v0);
            realsend(sendstr, is, Type::notify_u_f);
            break;

        case Type::notify_g_req:
            set_IDs(&sendstr,{id,id2});
            realsend(sendstr, is, Type::notify_g_req);
            break;

        case Type::notify_g_m:
            set_Message(&sendstr,{id},v0,{},id2);
            realsend(sendstr, is, Type::notify_g_m);
            break;

        case Type::notify_g_f:
            set_File(&sendstr,{id},v0,{},{},id2);
            realsend(sendstr, is, Type::notify_g_f);
            break;

        case Type::fri_confirm:
            realsend(sendstr, is, Type::fri_confirm);
            break;

        case Type::g_confirm:
            realsend(sendstr, is, Type::g_confirm);
            break;

        default:
            break;
    }
    if(type!=Type::u_f_history1&&type!=Type::g_f_history1){
        tmp_lock.unlock();
        std::unique_lock<std::mutex> que_lock(que_mtx);
        std::vector<std::tuple<Type ,bool ,std::vector<string> ,std::vector<string> ,
                std::vector<string> ,std::vector<string> ,int ,int >>exec_que;
        for(auto &tmp:wait_que){
            exec_que.push_back(tmp);
        }
        wait_que.clear();
        que_lock.unlock();
        for(auto &tmp:exec_que){
            _send(std::get<0>(tmp),std::get<1>(tmp),std::get<2>(tmp),std::get<3>(tmp),std::get<4>(tmp),std::get<5>(tmp),std::get<6>(tmp),std::get<7>(tmp));
        }
    }
}
void Clannel_checked::deal()
{
    std::cerr<<"处理fd"<<fd<<std::endl;
    if(revent&EPOLLRDHUP)
    {
        std::cerr<<"因为对端关闭";
        quit();
        return;
    }
    else if(revent&EPOLLIN||revent&EPOLLOUT)
    {
        if(recv_cache()==0)return;
        else
        {
            chatroom::File _file;
            chatroom::Message _mess;
            chatroom::IDs _id;
            chatroom::Group_uid _group;
            chatroom::Strs _str;
            std::vector<std::vector<string>> tmpvv;
            std::vector<string> tmpv;
            std::cerr<<head.DebugString();
            int tmpi;
            bool is=1;
            long recvd;
            bool ret=0;
            long tmp;
            int sfd=-1;
            try{
                unique_ptr<char[]> ptr;
                switch (head.type())
                {
                    case Type::u_search:
                        tmpvv=reactor->DB.u_search(uid);
                        if(reactor->DB.mysql_iserr())is=0;
                        _send(Type::u_search,is,tmpvv[0],tmpvv[1]);
                        break;

                    case Type::u_request:
                        _id.ParseFromString(cache);
                        is=reactor->DB.u_request(uid,_id.id(0));
                        _send(Type::u_request,is);
                        if(is)
                        {
                            for(auto&tmp:reactor->reactors)
                            {
                                tmp->slock.lock();
                                try{
                                    tmp->fd_map.at(uid_map.at(_id.id(0))).clannel->
                                    _send(Type::notify_u_req,is,{},{},{},{},uid);
                                }catch(std::out_of_range){}
                                tmp->slock.unlock();
                            }
                        }
                        break;

                    case Type::u_listreq:
                        tmpvv = reactor->DB.u_listreq(uid);
                        if(reactor->DB.mysql_iserr()){
                            is = 0;
                            _send(Type::u_listreq, is, tmpv);
                        }else{
                            for(int c=0;c<tmpvv[0].size();c++){
                                tmpv.push_back(tmpvv[0][c]);
                                tmpv.push_back(tmpvv[1][c]);
                            }
                            _send(Type::u_listreq, is, tmpv);
                        }
                        break;

                    case Type::u_add:
                        _id.ParseFromString(cache);
                        is=reactor->DB.u_add(uid,_id.id(0));
                        _send(Type::u_add,is);
                        break;

                    case Type::u_del:
                        _id.ParseFromString(cache);
                        is=reactor->DB.u_del(uid,_id.id(0));
                        _send(Type::u_del,is);
                        break;

                    case Type::u_blok:
                        _id.ParseFromString(cache);
                        is=reactor->DB.u_blok(uid,_id.id(0));
                        _send(Type::u_blok,is);
                        break;

                    case Type::u_unblok:
                        _id.ParseFromString(cache);
                        is=reactor->DB.u_unblok(uid,_id.id(0));
                        _send(Type::u_unblok,is);
                        break;

                    case Type::u_message:
                        _mess.ParseFromString(cache);
                        is=reactor->DB.u_message(uid,_mess.obj(0),_mess.context(0));
                        _send(Type::u_message,is);
                        if(is)
                        {
                            for(auto&tmp:reactor->reactors)
                            {
                                tmp->slock.lock();
                                try{
                                    tmp->fd_map.at(uid_map.at(_mess.obj(0))).clannel->
                                    _send(Type::notify_u_m,is,{_mess.context(0)},{},{},{},uid);
                                }catch(std::out_of_range){}
                                tmp->slock.unlock();
                            }
                        }
                        break;

                    case Type::u_file:
                        _file.ParseFromString(cache);
                        reactor->DB.is_fri(uid,_file.obj(0));
                        ptr.reset(new char[40960]);
                        if (need_recv==0){
                            rm_in_chatroom(_file.name(0),uid,_file.obj(0));
                            need_recv=_file.len(0);
                        }
                        while(need_recv!=0){
                            if(need_recv>40960){
                                recvd=recv(fd,ptr.get(),40960,MSG_DONTWAIT);
                                if (recvd<40960){
                                    ret=1;
                                }
                            }else{
                                recvd=recv(fd,ptr.get(),need_recv,MSG_DONTWAIT);
                                if (recvd<need_recv){
                                    ret=1;
                                }
                            }
                            if(recvd>0){
                                tmp=append_in_chatroom(ptr.get(),recvd,_file.name(0),uid,_file.obj(0));
                                if (tmp<recvd){
                                    std::cerr<<"文件append错误\n";
                                    rm_in_chatroom(_file.name(0),uid,_file.obj(0));
                                    is=0;
                                    break;
                                }else{
                                    need_recv-=tmp;
                                }
                            }else{
                                rm_in_chatroom(_file.name(0),uid,_file.obj(0));
                                is=0;
                                break;
                            }
                            
                            if(ret){
                                return;
                            }
                        }
                        
                        if(is){
                            is=reactor->DB.u_file(uid,_file.obj(0),_file.len(0),_file.name(0));
                        }
                        _send(Type::u_file,is);
                        if(is)
                        {
                            for(auto&tmp:reactor->reactors)
                            {
                                tmp->slock.lock();
                                try{
                                    tmp->fd_map.at(uid_map.at(_file.obj(0))).clannel->
                                    _send(Type::notify_u_f,is,{_file.name(0)},{},{},{},uid);                                    
                                }catch(std::out_of_range){}
                                tmp->slock.unlock();
                            }
                        }else{
                            rm_in_chatroom(_file.name(0),uid,_file.obj(0));
                        }
                        break;

                    case Type::u_m_history:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.u_m_history(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::u_m_history, is, tmpvv[0], tmpvv[1],tmpvv[2]);
                        break;

                    case Type::u_f_history0:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.u_f_history0(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::u_f_history0, is, tmpvv[0], tmpvv[1],tmpvv[2]);
                        break;

                    case Type::u_f_history1:
                        _file.ParseFromString(cache);
                        if(need_send==0){
                            tmpvv = reactor->DB.u_f_history1(uid,_file.obj(0),_file.name(0));
                            if(reactor->DB.mysql_iserr()||tmpvv[0].size()==0)is = 0;
                            _send(Type::u_f_history1, is, tmpvv[0],tmpvv[1],tmpvv[2],tmpvv[3]);
                            if(is){
                                int fl_flags=fcntl(fd,F_GETFL);
                                if(fl_flags==-1)std::cerr<<"flags错误";
                                fl_flags|=O_NONBLOCK;
                                if(fcntl(fd,F_SETFL,fl_flags)<0)std::cerr<<"非阻塞设置错误";
                                need_send=std::stol(tmpvv[2][0]);
                                reactor->add_epollout(fd);
                                mtx.lock();
                            }else{
                                break;
                            }
                        }

                        sfd=open(("/var/lib/chatroom_files/u"+std::to_string(uid)+std::to_string(_file.obj(0))+_file.name(0)).c_str(),O_RDONLY);
                        if(sfd<0){
                            sfd=open(("/var/lib/chatroom_files/u"+std::to_string(_file.obj(0))+std::to_string(uid)+_file.name(0)).c_str(),O_RDONLY);
                        }
                        if(sfd>=0){
                            tmp=sendfile64(fd,sfd,offset.get(),need_send);
                            if(tmp>=0||errno==EAGAIN){
                                std::cerr<<"一次发了"<<tmp<<"\n";
                                need_send-=tmp;
                                if(need_send>0){
                                    close(sfd);
                                    return;
                                }else if(need_send==0){// 结束
                                    std::cerr<<"发完\n";
                                    int fl_flags=fcntl(fd,F_GETFL);
                                    if(fl_flags==-1)std::cerr<<"结束时flags错误";
                                    fl_flags&=~O_NONBLOCK;
                                    if(fcntl(fd,F_SETFL,fl_flags)<0)std::cerr<<"阻塞设置错误";
                                    reactor->rm_epollout(fd);
                                    close(sfd);
                                    mtx.unlock();
                                    std::unique_lock<std::mutex> que_lock(que_mtx);
                                    std::vector<std::tuple<Type ,bool ,std::vector<string> ,std::vector<string> ,
                                            std::vector<string> ,std::vector<string> ,int ,int >>exec_que;
                                    for(auto &tmp:wait_que){
                                        exec_que.push_back(tmp);
                                    }
                                    wait_que.clear();
                                    que_lock.unlock();
                                    for(auto &tmp:exec_que){
                                        _send(std::get<0>(tmp),std::get<1>(tmp),std::get<2>(tmp),std::get<3>(tmp),
                                            std::get<4>(tmp),std::get<5>(tmp),std::get<6>(tmp),std::get<7>(tmp));
                                    }
                                    break ;
                                }else{
                                    mtx.unlock();
                                    close(sfd);
                                    quit();
                                    std::cerr<<"need_send<0\n";
                                    break;
                                }
                            }else{
                                std::cerr<<errno<<"sendfile返回负值且不是EAGIN\n";
                                mtx.unlock();
                                close(sfd);
                                quit();
                            }
                        }else{
                            std::cerr<<"文件打开失败\n";
                            mtx.unlock();
                            quit();
                        }
                        break;

                    case Type::g_create:
                        _group.ParseFromString(cache);
                        tmpi = reactor->DB.g_create(uid,_group.name());
                        if(tmpi== 0)_send(Type::g_create,0);
                        else{
                            _send(Type::g_create,1,{},{},{},{},tmpi);
                        }
                        break;

                    case Type::g_disban:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_disban(uid,_id.id(0));
                        _send(Type::g_disban,is);
                        break;

                    case Type::g_request:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_request(uid,_id.id(0));
                        _send(Type::g_request,is);
                        if(is)
                        {                
                            auto managers=reactor->DB.g_manager(_id.id(0));
                            for(auto&tmp:managers)
                            {
                                for(auto&tmp2:reactor->reactors)
                                {
                                    tmp2->slock.lock();
                                    try{
                                        tmp2->fd_map.at(uid_map.at(tmp)).clannel->
                                        _send(Type::notify_g_req,is,{},{},{},{},uid,_id.id(0));
                                    }catch(std::out_of_range){}
                                    tmp2->slock.unlock();
                                }
                            }  
                        }
                        break;

                    case Type::g_listreq:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.g_listreq(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr()){
                            is = 0;
                            _send(Type::g_listreq, is, tmpv);
                        }else{
                            for(int c=0;c<tmpvv[0].size();c++){
                                tmpv.push_back(tmpvv[0][c]);
                                tmpv.push_back(tmpvv[1][c]);
                            }
                            _send(Type::g_listreq, is, tmpv);
                        }
                        break;

                    case Type::g_add:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_add(uid,_id.id(0),_id.id(1));
                        _send(Type::g_add,is);
                        break;

                    case Type::g_del:
                        _id.ParseFromString(cache);
                        if(uid==_id.id(1)){
                            _send(Type::g_del,0);
                        }else{
                            is=reactor->DB.g_del(uid,_id.id(0),_id.id(1));
                            _send(Type::g_del,is);
                        }
                        break;

                    case Type::g_search:
                        tmpvv = reactor->DB.g_search(uid);
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::g_search, is, tmpvv[0], tmpvv[1]);
                        break;

                    case Type::g_message:
                        _mess.ParseFromString(cache);
                        is=reactor->DB.g_message(uid,_mess.gid(),_mess.context(0));
                        _send(Type::g_message,is);
                        if(is)
                        {                
                            auto members=reactor->DB.g_members(_mess.gid());
                            for(auto&tmp:members)
                            {
                                for(auto&tmp2:reactor->reactors)
                                {
                                    tmp2->slock.lock();
                                    try{
                                        tmp2->fd_map.at(uid_map.at(tmp)).clannel->
                                        _send(Type::notify_g_m,is,{_mess.context(0)},{},{},{},uid,_mess.gid());
                                    }catch(std::out_of_range){}
                                    tmp2->slock.unlock();
                                }
                            }
                        }
                        break;

                    case Type::g_file:
                        _file.ParseFromString(cache);
                        reactor->DB.is_gmember(uid,_file.obj(0));
                        ptr.reset(new char[40960]);
                        if (need_recv==0){
                            rm_in_chatroom(_file.name(0),uid,_file.obj(0));
                            need_recv=_file.len(0);
                        }
                        while(need_recv!=0){
                            if(need_recv>40960){
                                recvd=recv(fd,ptr.get(),40960,MSG_DONTWAIT);
                                if (recvd<40960){
                                    ret=1;
                                }
                            }else{
                                recvd=recv(fd,ptr.get(),need_recv,MSG_DONTWAIT);
                                if (recvd<need_recv){
                                    ret=1;
                                }
                            }
                            if(recvd>0){
                                tmp=append_in_chatroom(ptr.get(),recvd,_file.name(0),_file.obj(0));
                                if (tmp<recvd){
                                    std::cerr<<"文件append错误\n";
                                    rm_in_chatroom(_file.name(0),_file.obj(0));
                                    is=0;
                                    break;
                                }else{
                                    need_recv-=tmp;
                                }
                            }else{
                                rm_in_chatroom(_file.name(0),_file.obj(0));
                                is=0;
                                break;
                            }
                            
                            if(ret){
                                return;
                            }
                        }

                        if(is){
                            is=reactor->DB.g_file(uid,_file.obj(0),_file.len(0),_file.name(0));
                        }
                        _send(Type::g_file,is);
                        if(is)
                        {                
                            auto members=reactor->DB.g_members(_file.obj(0));
                            for(auto&tmp:members)
                            {
                                for(auto&tmp2:reactor->reactors)
                                {
                                    tmp2->slock.lock();
                                    try{
                                        tmp2->fd_map.at(uid_map.at(tmp)).clannel->
                                        _send(Type::notify_g_f,is,{_file.name(0)},{},{},{},uid,_file.obj(0));
                                    }catch(std::out_of_range){}
                                    tmp2->slock.unlock();
                                }
                            }  
                        }else{
                            rm_in_chatroom(_file.name(0),_file.obj(0));
                        }
                        break;

                    case Type::g_quit:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_quit(uid,_id.id(0));
                        _send(Type::g_quit,is);
                        break;

                    case Type::g_members:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.g_members(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::g_members, is, tmpvv[0], tmpvv[1],tmpvv[2]);
                        break;

                    case Type::g_addmanager:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_addmanager(uid,_id.id(0),_id.id(1));
                        _send(Type::g_addmanager,is);
                        break;

                    case Type::g_delmanager:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_delmanager(uid,_id.id(0),_id.id(1));
                        _send(Type::g_delmanager,is);
                        break;

                    case Type::g_m_history:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.g_m_history(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::g_m_history, is, tmpvv[0], tmpvv[1],tmpvv[2]);
                        break;

                    case Type::g_f_history0:
                        _id.ParseFromString(cache);
                        tmpvv = reactor->DB.g_f_history0(uid,_id.id(0));
                        if(reactor->DB.mysql_iserr())is = 0;
                        _send(Type::g_f_history0, is, tmpvv[0], tmpvv[1],tmpvv[2]);
                        break;

                    case Type::g_f_history1:
                        _file.ParseFromString(cache);
                        if(need_send==0){
                            tmpvv = reactor->DB.g_f_history1(uid,_file.obj(0),_file.name(0));
                            if(reactor->DB.mysql_iserr()||tmpvv[0].size()==0)is = 0;
                            _send(Type::g_f_history1, is, tmpvv[0],tmpvv[1],tmpvv[2],tmpvv[3]);
                            if(is){
                                int fl_flags=fcntl(fd,F_GETFL);
                                if(fl_flags==-1)std::cerr<<"flags错误";
                                fl_flags|=O_NONBLOCK;
                                if(fcntl(fd,F_SETFL,fl_flags)<0)std::cerr<<"非阻塞设置错误";
                                need_send=std::stol(tmpvv[2][0]);
                                reactor->add_epollout(fd);
                                mtx.lock();
                            }else{
                                break;
                            }
                        }
                        
                        sfd=open(("/var/lib/chatroom_files/g"+std::to_string(_file.obj(0))+_file.name(0)).c_str(),O_RDONLY);
                        if(sfd>=0){
                            tmp=sendfile64(fd,sfd,offset.get(),need_send);
                            if(tmp>=0||errno==EAGAIN){
                                std::cerr<<"一次发了"<<tmp<<"\n";
                                need_send-=tmp;
                                if(need_send>0){
                                    close(sfd);
                                    return;
                                }else if(need_send==0){// 结束
                                    std::cerr<<"发完\n";
                                    int fl_flags=fcntl(fd,F_GETFL);
                                    if(fl_flags==-1)std::cerr<<"结束时flags错误";
                                    fl_flags&=~O_NONBLOCK;
                                    if(fcntl(fd,F_SETFL,fl_flags)<0)std::cerr<<"阻塞设置错误";
                                    reactor->rm_epollout(fd);
                                    close(sfd);
                                    mtx.unlock();
                                    std::unique_lock<std::mutex> que_lock(que_mtx);
                                    std::vector<std::tuple<Type ,bool ,std::vector<string> ,std::vector<string> ,
                                            std::vector<string> ,std::vector<string> ,int ,int >>exec_que;
                                    for(auto &tmp:wait_que){
                                        exec_que.push_back(tmp);
                                    }
                                    wait_que.clear();
                                    que_lock.unlock();
                                    for(auto &tmp:exec_que){
                                        _send(std::get<0>(tmp),std::get<1>(tmp),std::get<2>(tmp),std::get<3>(tmp),
                                            std::get<4>(tmp),std::get<5>(tmp),std::get<6>(tmp),std::get<7>(tmp));
                                    }
                                    break ;
                                }else{
                                    mtx.unlock();
                                    close(sfd);
                                    quit();
                                    std::cerr<<"need_send<0\n";
                                    break;
                                }
                            }else{
                                std::cerr<<errno<<"sendfile返回负值且不是EAGIN\n";
                                mtx.unlock();
                                close(sfd);
                                quit();
                            }
                        }else{
                            std::cerr<<"文件打开失败\n";
                            mtx.unlock();
                            quit();
                        }
                        break;

                    case Type::g_confirm:
                        _id.ParseFromString(cache);
                        is=reactor->DB.g_confirm(uid,_id.id(0));
                        _send(Type::g_confirm,is);
                        break;

                    case Type::fri_confirm:
                        _id.ParseFromString(cache);
                        is=reactor->DB.fri_confirm(uid,_id.id(0));
                        _send(Type::fri_confirm,is);
                        break;

                    case Type::heart_check:
                        break;
                    
                    default:
                        quit();
                        return;
                }
            }catch(select_err){
                std::cerr<<"select错误";
                _send(head.type(),0);
            }
            need_recv=0;
            need_send=0;
            *offset=0;
            reset();
        }
    }
}
void Clannel_checked::quit()
{
    if(need_recv>0){
        chatroom::File _file;
        _file.ParseFromString(cache);
        if(head.type()==Type::u_file){
            rm_in_chatroom(_file.name(0),uid,_file.obj(0));
        }else{
            rm_in_chatroom(_file.name(0),_file.obj(0));
        }
    }
    uid_ulock.lock();
    uid_map.erase(uid);
    uid_ulock.unlock();
    reactor->reducefd(fd);
    close(fd);
    std::cerr<<"将"<<fd<<"断开连接";
}

//Clannel_nocheck
void Clannel_nocheck::_send(Type type,bool is,int id,std::vector<string> v)
{
    char len;
    string _head;
    if(is==0)std::cerr<<reactor->DB.mysql_err()<<"\n";
    if(type==Type::login)
    {
        if(is==0)
        {
            set_Head(&_head,is,Type::login,0);
            len=_head.size();
            send(fd,(string(&len,sizeof(len))+_head).c_str(),len+sizeof(len),0);
            return;
        }
        string _id;
        std::vector<int> tmparg;
        for(int c=0;c<v.size();c++)
        {
            tmparg.push_back(std::stoi(v[c]));
        }
        set_IDs(&_id,tmparg);
        set_Head(&_head,is,Type::login,_id.size());
        len=_head.size();
        send(fd,(string(&len,sizeof(len))+_head).c_str(),len+sizeof(len),0);
        send(fd,_id.c_str(),_id.size(),0);
    }
    else if(type==Type::signup)
    {
        if(is==0)
        {
            set_Head(&_head,is,Type::signup,0);
            len=_head.size();
            send(fd,(string(&len,sizeof(len))+_head).c_str(),len+sizeof(len),0);
            return;
        }
        string _signup;
        set_Signup_info(&_signup,id);
        set_Head(&_head,is,Type::signup,_signup.size());
        len=_head.size();
        send(fd,(string(&len,sizeof(len))+_head).c_str(),len+sizeof(len),0);
        send(fd,_signup.c_str(),_signup.size(),0);
    }
    else if(type==Type::logout)
    {
        set_Head(&_head,is,Type::logout,0);
        len=_head.size();
        send(fd,(string(&len,sizeof(len))+_head).c_str(),len+sizeof(len),0);
    }
}
void Clannel_nocheck::deal()
{
    std::cerr<<"处理fd"<<fd;
    if(revent&EPOLLRDHUP)
    {
        std::cerr<<"因为对端关闭";
        quit();
        return;
    }
    else if(revent&EPOLLIN)
    {
        if(recv_cache()==0)return;
        else
        {
            chatroom::Login_info login;
            chatroom::Signup_info signup;
            int tmp;
            try{
                switch (head.type())
                {
                    case Type::login:
                        login.ParseFromString(cache);
                        if((Clannel_checked::uid_map.find(login.uid())!=Clannel_checked::uid_map.cend())||
                            (reactor->DB.login(login.uid(),login.password())==0))_send(Type::login,0);
                        else
                        {
                            reactor->modfd(fd,Reactor_type::checked,login.uid());
                            _send(Type::login,1,0,reactor->DB.search_mess(login.uid()));
                        }
                        break;
                    
                    case Type::signup:
                        signup.ParseFromString(cache);
                        if((tmp=reactor->DB.signup(signup.name(),signup.password()))==0)_send(Type::signup,0);
                        else _send(Type::signup,1,tmp);
                        break;

                    case Type::logout:
                        login.ParseFromString(cache);
                        _send(Type::logout,reactor->DB.logout(login.uid(),login.password()));
                        break;

                    case Type::heart_check:
                        break;

                    default:
                        quit();
                        return;
                }
            }catch(select_err){
                std::cerr<<"select错误";
                _send(head.type(),0);
            }
            reset();
        }
    }
}
void Clannel_nocheck::quit()
{
    reactor->reducefd(fd);
    close(fd);
    std::cerr<<"将"<<fd<<"断开连接";
}



#endif
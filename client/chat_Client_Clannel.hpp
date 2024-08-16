
#ifndef chat_Client_Clannel
#define chat_Client_Clannel
#include"../chatroom.pb.h"
#include<string>
#include<mutex>
#include<condition_variable>
#include<unistd.h>
#include<sys/socket.h>
#include<tuple>
#include<fstream>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/stat.h>
using std::string;
using chatroom::Type;
#define pr printf("=========================================================================\n")
#define spr printf("-----------------------------------------------------------------------\n")
#define _Greed "\033[32m"
#define _Yellow "\033[33m"
#define _Blue "\033[34m"
#define _Reset "\033[0m "
class Clannel{
public:
    Clannel(int _fd):fd(_fd),ulock(mtx,std::defer_lock){}
    void chfd(int _fd)
    {
        fd=_fd;
    }
    bool ret()
    {
        return recv_ret;
    }
    ~Clannel(){}
    static int uid;
    static int in_uid;
    static int in_gid;
protected:
    int fd=-1;
    static bool send_continue;
    static bool recv_ret;
    static std::mutex mtx;
    std::unique_lock<std::mutex> ulock;
    static std::condition_variable cond;
};
std::mutex Clannel::mtx;
bool Clannel::send_continue=0;
bool Clannel::recv_ret=0;
int Clannel::uid=0;
int Clannel::in_uid=0;
int Clannel::in_gid=0;
std::condition_variable Clannel::cond;



void set_Head(string*s_ptr,Type type,int len)
{
    chatroom::Head _Head;
    _Head.set_len(len);
    _Head.set_type(type);
    _Head.SerializeToString(s_ptr);
}
void set_File(string*s_ptr,const std::vector<int> &obj,const std::vector<string>& name,const std::vector<string>& context={})
{
    chatroom::File _File;
    for(auto &tmp:obj)_File.add_obj(tmp);
    for(auto &tmp:name)_File.add_name(tmp);
    for(auto &tmp:context)_File.add_len(std::stol(tmp));
    _File.SerializePartialToString(s_ptr);
}
void set_Message(string*s_ptr,const std::vector<int>&obj={},const std::vector<string>&context={},int gid=0)
{
    chatroom::Message _Message;
    for(auto &tmp:obj)_Message.add_obj(tmp);
    for(auto &tmp:context)_Message.add_context(tmp);
    if(gid!=0)_Message.set_gid(gid);
    _Message.SerializePartialToString(s_ptr);
}
void set_IDs(string*s_ptr,const std::vector<int>&id)
{
    chatroom::IDs _IDs;
    for(auto &tmp:id)_IDs.add_id(tmp);
    _IDs.SerializePartialToString(s_ptr);
}
void set_Signup_info(string*s_ptr,const string&name,const string&password)
{
    chatroom::Signup_info _Signup;
    _Signup.set_name(name);
    _Signup.set_password(password);
    _Signup.SerializePartialToString(s_ptr);
}
void set_Login_info(string*s_ptr,const int&uid,const string&password)
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

#endif
#ifndef chat_Client_Clannel_send
#define chat_Client_Clannel_send
#include"chat_Client_Clannel.hpp"
#include<filesystem>

class Clannel_send:public Clannel{
public:
    Clannel_send(int _fd=-1):Clannel(_fd){}
    template<typename ...T>
    void _send(Type,int id1=0,int id2=0,const string&s1="",const string&s2="");
    ~Clannel_send(){}
    std::mutex mtx;
private:
    void realsend(const string& sendstr,Type type);
};
template<typename ...T>
void Clannel_send::_send(Type type,int id1,int id2,const string&s1,const string&s2)
{
    send_continue=0;
    string sendstr;
    int in_fd;
    ssize_t sent=0;
    ssize_t send_ret;
    std::unique_ptr<loff_t> u_ptr;
    std::unique_lock<std::mutex> tmp_lock(mtx);
    switch (type)
    {
        case Type::login:
            uid=id1;
            set_Login_info(&sendstr,id1,s1);
            realsend(sendstr,Type::login);
            break;

        case Type::signup:
            set_Signup_info(&sendstr,s1,s2);
            realsend(sendstr,Type::signup);
            break;

        case Type::logout:
            set_Login_info(&sendstr,id1,s1);
            realsend(sendstr,Type::logout);
            break;

        case Type::u_search:
            realsend("",Type::u_search);
            break;
    
        case Type::u_request:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_request);
            break;
        
        case Type::u_listreq:
            realsend("",Type::u_listreq);
            break;
        
        case Type::u_add:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_add);
            break;
        
        case Type::u_del:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_del);
            break;
        
        case Type::u_blok:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_blok);
            break;
        
        case Type::u_unblok:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_unblok);
            break;
        
        case Type::u_message:
            set_Message(&sendstr,{id1},{s1});
            realsend(sendstr,Type::u_message);
            break;
        
        case Type::u_file:
            if((in_fd=open(s2.c_str(),O_RDONLY))<0){
                std::cerr<<"文件无法打开\n";
                return ;
            }
            set_File(&sendstr,{id1},{s1},{std::to_string(lseek64(in_fd,0,SEEK_END))});
            realsend(sendstr,Type::u_file);
            u_ptr.reset(new loff_t(0));
            while(true){
                send_ret=sendfile64(fd,in_fd,u_ptr.get(),lseek64(in_fd,0,SEEK_END)-sent);
                if(send_ret<0){
                    std::cerr<<"文件发送失败\n";
                    break;
                }else{
                    sent+=send_ret;
                }
                if((std::filesystem::file_size(s2)-sent)==0){
                    break;
                }
            }
            break;
        
        case Type::u_m_history:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_m_history);
            break;
        
        case Type::u_f_history0:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::u_f_history0);
            break;
        
        case Type::u_f_history1:
            set_File(&sendstr,{id1},{s1});
            realsend(sendstr,Type::u_f_history1);
            break;
        
        case Type::g_create:
            set_Group_uid(&sendstr,0,s1);
            realsend(sendstr,Type::g_create);
            break;
        
        case Type::g_disban:
            set_Group_uid(&sendstr,id1);
            realsend(sendstr,Type::g_disban);
            break;
        
        case Type::g_request:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_request);
            break;
        
        case Type::g_listreq:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_listreq);
            break;
        
        case Type::g_add:
            set_IDs(&sendstr,{id1,id2});
            realsend(sendstr,Type::g_add);
            break;
        
        case Type::g_del:
            set_IDs(&sendstr,{id1,id2});
            realsend(sendstr,Type::g_del);
            break;
        
        case Type::g_search:
            realsend(sendstr,Type::g_search);
            break;
        
        case Type::g_message:
            set_Message(&sendstr,{id1},{s1},id2);
            realsend(sendstr,Type::g_message);
            break;
        
        case Type::g_file:
            if((in_fd=open(s2.c_str(),O_RDONLY))<0){
                std::cerr<<"文件无法打开\n";
                return ;
            }
            set_File(&sendstr,{id1},{s1},{std::to_string(std::filesystem::file_size(s2))});
            realsend(sendstr,Type::g_file);
            u_ptr.reset(new loff_t(0));
            while(true){
                send_ret=sendfile64(fd,in_fd,u_ptr.get(),std::filesystem::file_size(s2)-sent);
                if(send_ret<0){
                    std::cerr<<"文件发送失败\n";
                    break;
                }else{
                    sent+=send_ret;
                }
                if((std::filesystem::file_size(s2)-sent)==0){
                    break;
                }
            }
            break;
        
        case Type::g_quit:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_quit);
            break;
        
        case Type::g_members:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_members);
            break;
        
        case Type::g_addmanager:
            set_IDs(&sendstr,{id1,id2});
            realsend(sendstr,Type::g_addmanager);
            break;
        
        case Type::g_delmanager:
            set_IDs(&sendstr,{id1,id2});
            realsend(sendstr,Type::g_delmanager);
            break;
        
        case Type::g_m_history:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_m_history);
            break;
        
        case Type::g_f_history0:
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_f_history0);
            break;
        
        case Type::g_f_history1:
            set_File(&sendstr,{id1},{s1});
            realsend(sendstr,Type::g_f_history1);
            break;

        case Type::g_confirm:
            in_gid=id1;
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::g_confirm);
            break;

        case Type::fri_confirm:
            in_uid=id1;
            set_IDs(&sendstr,{id1});
            realsend(sendstr,Type::fri_confirm);
            break;

        default:
            break;
    }
    if(type!=Type::u_message&&type!=Type::g_message){
        std::cerr<<"等待响应...\n";
    }
    ulock.lock();
    cond.wait(ulock,[](){return send_continue==1;});
    ulock.unlock();
}
void  Clannel_send::realsend(const string &sendstr,Type type)
{
    string sendhead;
    set_Head(&sendhead,type,sendstr.size());
    char len=sendhead.size();
    if(send(fd,(string(&len,sizeof(len))+sendhead).c_str(),sizeof(len)+len,0)==-1)exit(EXIT_FAILURE);
    if(sendstr.size()>0)if(send(fd,sendstr.c_str(),sendstr.size(),0)==-1)exit(EXIT_FAILURE);
}

#endif

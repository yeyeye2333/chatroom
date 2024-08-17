#ifndef chat_Client_Clannel_recv
#define chat_Client_Clannel_recv
#include"chat_Client_Clannel.hpp"

class Clannel_recv:public Clannel{
public:
    Clannel_recv(int _fd=-1):Clannel(_fd){}
    void _recv();
    ~Clannel_recv(){}
private:
};
void Clannel_recv::_recv()
{
    char len;
    if(recv(fd,&len,sizeof(len),0)<sizeof(len))
    {
        std::cerr<<"与服务器断开连接1";
        exit(EXIT_FAILURE);
    }
    char tmp[len];
    if(recv(fd,tmp,len,0)<len)
    {
        std::cerr<<"与服务器断开连接2";
        exit(EXIT_FAILURE);
    }
    chatroom::Head _head;
    _head.ParseFromArray(tmp,len);
// std::cerr<<_head.DebugString();
    char tmp2[_head.len()];
    if(_head.len()>0)
    {
        int recvd=0;
        int tmp;
        while(true)
        {
            tmp=recv(fd,tmp2+recvd,_head.len(),0);
            if(tmp<0){
                std::cerr<<errno<<"错误码";
                std::cerr<<"与服务器断开连接3";
                exit(EXIT_FAILURE);
            }
            recvd+=tmp;
            if(recvd==_head.len())break;
        }
    }
    chatroom::File _file;
    chatroom::Message _mess;
    chatroom::Signup_info _signup;
    chatroom::Login_info _login;
    chatroom::IDs _id;
    chatroom::Strs _str;
    if(_head.type()!=Type::notify_g_f&&_head.type()!=Type::notify_g_m&&_head.type()!=Type::notify_g_req
    &&_head.type()!=Type::notify_u_f&&_head.type()!=Type::notify_u_m&&_head.type()!=Type::notify_u_req
    &&_head.type()!=Type::u_message&&_head.type()!=Type::g_message)
    {
        recv_ret=_head.is();
        spr;
    }
    std::ofstream file_o;
    string file_path;
    switch(_head.type())
    {
        case Type::login:
            if(_head.is()==0)
            {
                uid=0;
                std::cerr<<"账户无法登录/密码错误\n";
            }
            else
            {
                _id.ParseFromArray(tmp2,_head.len());
                if(_id.id_size()>0)std::cout<<"离线时有用户发送消息:"<<std::endl;
                for(int c=0;c<_id.id_size();c++)std::cout<<_id.id(c)<<std::endl;
            }
            break;

        case Type::signup:
            if(_head.is()==0)std::cerr<<"注册失败\n";
            else
            {
                _signup.ParseFromArray(tmp2,_head.len());
                std::cout<<"注册成功,你的 uid="<<_signup.uid()<<std::endl;
            }
            break;

        case Type::logout:
            if(_head.is()==0)std::cerr<<"注销失败,账号/密码错误\n";
            else std::cout<<"注册成功"<<std::endl;
            break;
            
        case Type::u_search:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _mess.ParseFromArray(tmp2,_head.len());
                std::cout<<"用户\t\tID"<<std::endl;
                for(int c=0;c<_mess.obj_size();c++)
                {
                    std::cout<<_mess.context(c)<<"\t\t"<<_mess.obj(c)<<std::endl;
                }
            }
            break;
    
        case Type::u_request:
            if(_head.is()==0)std::cerr<<"操作失败/已申请\n";
            else std::cout<<"请求成功"<<std::endl;
            break;
        
        case Type::u_listreq:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _str.ParseFromArray(tmp2,_head.len());
                std::cout<<"请求ID\t\t名字\n";
                for(int c=0;c<_str.str_size();c+=2)
                {
                    std::cout<<_str.str(c)<<"\t\t"<<_str.str(c+1)<<'\n';
                }
                std::cout<<std::endl;
            }
            break;
        
        case Type::u_add:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"添加成功"<<std::endl;
            break;
        
        case Type::u_del:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"对方已不是好友"<<std::endl;
            break;
        
        case Type::u_blok:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"屏蔽成功"<<std::endl;
            break;
        
        case Type::u_unblok:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"已解除屏蔽"<<std::endl;
            break;
        
        case Type::u_message:
            if(_head.is()==0)std::cerr<<"操作失败/被屏蔽\n";
            break;
        
        case Type::u_file:
            if(_head.is()==0)std::cerr<<"操作失败/被屏蔽\n";
            else std::cout<<"发送成功"<<std::endl;
            break;
        
        case Type::u_m_history:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _mess.ParseFromArray(tmp2,_head.len());
                for(int c=0;c<_mess.obj_size();c++)
                {
                    std::cout<<_mess.obj(c)<<":\t"<<_mess.context(c)<<"\ttime:"<<_mess.date(c)<<std::endl;
                }
            }
            break;
        
        case Type::u_f_history0:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _file.ParseFromArray(tmp2,_head.len());
                for(int c=0;c<_file.obj_size();c++)
                {
                    std::cout<<_file.obj(c)<<":\t"<<_file.name(c)<<"\ttime:"<<_file.date(c)<<std::endl;
                }
            }
            break;
        
        case Type::u_f_history1:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _file.ParseFromArray(tmp2,_head.len());
                std::cout<<"输入文件路径(目录):"<<std::flush;
                std::getline(std::cin,file_path);
                file_path+="/"+_file.name(0);
                try{
                    file_o.open(file_path,std::iostream::binary|std::iostream::out);
                    long need_recv=_file.len(0);
                    std::cerr<<"正在接收长度"<<_file.len(0)<<"的文件"<<std::endl;
                    unique_ptr<char[]>cache;
                    long len;
                    if(need_recv>100000){
                        cache.reset(new char[100000]);
                        len=100000;
                    }else{
                        cache.reset(new char[need_recv]);
                        len=need_recv;
                    }
                    if(file_o.is_open())
                    {
                        while (need_recv>0)
                        {
                            long tmp=recv(fd,cache.get(),len,0);
                            if (tmp<0)
                            {
                                continue;
                            }else{
                                need_recv-=tmp;
                                if(need_recv>100000){
                                    len=100000;
                                }else{
                                    len=need_recv;
                                }
                                file_o.write(cache.get(),tmp);
                            }
                        }
                        std::cout<<"接受完成"<<std::endl;
                    }
                }
                catch(std::invalid_argument){std::cerr<<"错误:文件名无效\n";}
                catch(std::ios_base::failure){std::cerr<<"错误:无权限\n";}
                catch(std::bad_alloc){std::cerr<<"错误:内存不足\n";}
            }
            break;
        
        case Type::g_create:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _id.ParseFromArray(tmp2,_head.len());
                std::cout<<"创建成功 gid="<<_id.id(0)<<std::endl;
            }
            break;
        
        case Type::g_disban:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"解散成功"<<std::endl;
            break;
        
        case Type::g_request:
            if(_head.is()==0)std::cerr<<"操作失败/已申请\n";
            else std::cout<<"请求成功"<<std::endl;
            break;
        
        case Type::g_listreq:
            if(_head.is()==0)std::cerr<<"操作失败/不是管理员\n";
            else
            {
                _str.ParseFromArray(tmp2,_head.len());
                std::cout<<"加群申请:\n";
                std::cout<<"请求ID\t\t名字\n";
                for(int c=0;c<_str.str_size();c+=2)
                {
                    std::cout<<_str.str(c)<<"\t\t"<<_str.str(c+1)<<'\n';
                }
                std::cout<<std::endl;
            }
            break;
        
        case Type::g_add:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"添加成功"<<std::endl;
            break;
        
        case Type::g_del:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"对方已不是群成员"<<std::endl;
            break;
        
        case Type::g_search:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _mess.ParseFromArray(tmp2,_head.len());
                std::cout<<"群名\t\tID\n";
                for(int c=0;c<_mess.obj_size();c++)
                {
                    std::cout<<_mess.context(c)<<"\t\t"<<_mess.obj(c)<<std::endl;
                }
            }
            break;
        
        case Type::g_message:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            break;
        
        case Type::g_file:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"发送成功"<<std::endl;
            break;
        
        case Type::g_quit:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"已退群"<<std::endl;
            break;
        
        case Type::g_members:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _mess.ParseFromArray(tmp2,_head.len());
                std::cout<<"名字\t\tUID\t\t是/否为管理员\n";
                for(int c=0;c<_mess.obj_size();c++)
                {
                    std::cout<<_mess.date(c)<<"\t\t";
                    std::cout<<_mess.obj(c)<<"\t\t";
                    if(_mess.context(c)=="1")std::cout<<"是";
                    else std::cout<<"不是";
                    std::cout<<"\n";
                }
                std::cout<<std::endl;
            }
            break;
        
        case Type::g_addmanager:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"设置成功"<<std::endl;
            break;
        
        case Type::g_delmanager:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else std::cout<<"取消成功"<<std::endl;
            break;
        
        case Type::g_m_history:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _mess.ParseFromArray(tmp2,_head.len());
                for(int c=0;c<_mess.obj_size();c++)
                {
                    std::cout<<_mess.obj(c)<<":\t"<<_mess.context(c)<<"\ttime:"<<_mess.date(c)<<std::endl;
                }
            }
            break;
        
        case Type::g_f_history0:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _file.ParseFromArray(tmp2,_head.len());
                for(int c=0;c<_file.obj_size();c++)
                {
                    std::cout<<_file.obj(c)<<":\t"<<_file.name(c)<<"\ttime:"<<_file.date(c)<<std::endl;
                }
            }
            break;
        
        case Type::g_f_history1:
            if(_head.is()==0)std::cerr<<"操作失败\n";
            else
            {
                _file.ParseFromArray(tmp2,_head.len());
                std::cout<<"输入文件路径(目录):"<<std::flush;
                std::cin>>file_path;
                file_path+="/"+_file.name(0);
                try{
                    file_o.open(file_path,std::iostream::binary|std::iostream::out);
                    long need_recv=_file.len(0);
                    unique_ptr<char[]>cache;
                    long len;
                    if(need_recv>100000){
                        cache.reset(new char[100000]);
                        len=100000;
                    }else{
                        cache.reset(new char[need_recv]);
                        len=need_recv;
                    }
                    if(file_o.is_open())
                    {
                        while (need_recv>0)
                        {
                            long tmp=recv(fd,cache.get(),len,MSG_DONTWAIT);
                            if (tmp<0)
                            {
                                continue;
                            }else{
                                need_recv-=tmp;
                                file_o.write(cache.get(),tmp);
                            }
                        }
                    }
                }
                catch(std::invalid_argument){std::cerr<<"错误:文件名无效\n";}
                catch(std::ios_base::failure){std::cerr<<"错误:无权限\n";}
                catch(std::bad_alloc){std::cerr<<"错误:内存不足\n";}
            }
            break;
        
        case Type::g_confirm:
            if(_head.is()==0)
            {
                in_gid=0;
                std::cerr<<"错误：不是组成员\n";
            }
            break;

        case Type::fri_confirm:
            if(_head.is()==0)
            {
                in_uid=0;
                std::cerr<<"错误：对方不是好友\n";
            }
            break;

        case Type::notify_u_req:
            _id.ParseFromArray(tmp2,_head.len());
            std::cout<<_Greed<<"\n\""<<_id.id(0)<<"\""<<"向你发送了好友请求"<<_Reset<<std::endl;
            break;
        
        case Type::notify_u_m:
            _mess.ParseFromArray(tmp2,_head.len());
            if(in_uid!=_mess.obj(0))std::cout<<_Blue<<"\n\""<<_mess.obj(0)<<"\""<<"向你发送了一条消息"<<_Reset<<std::endl;
            else std::cout<<_Blue<<"\n\""<<_mess.obj(0)<<"\":"<<_mess.context(0)<<_Reset<<std::endl;
            break;
        
        case Type::notify_u_f:
            _file.ParseFromArray(tmp2,_head.len());
            if(in_uid!=_file.obj(0))std::cout<<_Yellow<<"\n\""<<_file.obj(0)<<"\""<<"向你发送了一个文件"<<_Reset<<std::endl;
            else std::cout<<_Yellow<<"\n\""<<_file.obj(0)<<"\"->"<<_file.name(0)<<_Reset<<std::endl;
            break;
        
        case Type::notify_g_req:
            _id.ParseFromArray(tmp2,_head.len());
            std::cout<<_Greed<<"\n\""<<_id.id(0)<<"\""<<"向群\""<<_id.id(1)<<"\"发送了进群申请"<<_Reset<<std::endl;
            break;
        
        case Type::notify_g_m:
            _mess.ParseFromArray(tmp2,_head.len());
            if(in_gid!=_mess.gid())std::cout<<_Blue<<"\n\""<<_mess.obj(0)<<"\""<<"在群\""<<_mess.gid()<<"\"中发送了一条消息"<<_Reset<<std::endl;
            else std::cout<<_Blue<<"\n\""<<_mess.obj(0)<<"\":"<<_mess.context(0)<<_Reset<<std::endl;
            break;
        
        case Type::notify_g_f:
            _file.ParseFromArray(tmp2,_head.len());
            if(in_gid!=_file.gid())std::cout<<_Yellow<<"\n\""<<_file.obj(0)<<"\""<<"在群\""<<_file.gid()<<"\"中发送了一个文件"<<_Reset<<std::endl;
            else std::cout<<_Yellow<<"\n\""<<_file.obj(0)<<"\"->"<<_file.name(0)<<_Reset<<std::endl;
            break;

        default:
            break;
    }
    if(_head.type()!=Type::notify_g_f&&_head.type()!=Type::notify_g_m&&_head.type()!=Type::notify_g_req
    &&_head.type()!=Type::notify_u_f&&_head.type()!=Type::notify_u_m&&_head.type()!=Type::notify_u_req)send_continue=1;
    cond.notify_all();
}


#endif
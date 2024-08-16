#ifndef chat_Socket
#define chat_Socket
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<string>
#include<iostream>
class Socket_listen{
public:
    Socket_listen(int _domain=AF_INET6,int type=SOCK_STREAM|SOCK_CLOEXEC):domain(_domain)
    {
        fd=socket(domain,type,0);
    }
    ~Socket_listen()
    {
        if(fd!=-1)close(fd);
    }
    bool _bind(int port=50000);
    bool _listen()
    {
        return !listen(fd,SOMAXCONN);
    }
    int _accept(sockaddr* addr=nullptr,socklen_t len=0)
    {
        return accept4(fd,addr,&len,SOCK_CLOEXEC);
    }
private:
    int domain;
    int fd;
};
bool Socket_listen::_bind(int port)
{
    sockaddr_in6 in6;
    sockaddr_in in;
    if(domain==AF_INET6)
    {
        memset(&in6,0,sizeof(in6));
        in6.sin6_family=AF_INET6;
        in6.sin6_addr=in6addr_any;
        in6.sin6_port=htons(port);
        int optval=1;
        setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
        return !bind(fd,(sockaddr*)&in6,sizeof(in6));
    }
    else 
    {
        memset(&in,0,sizeof(in));
        in.sin_family=AF_INET;
        in.sin_addr.s_addr=INADDR_ANY;
        in.sin_port=htons(port);
        int optval=1;
        setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
        return !bind(fd,(sockaddr*)&in,sizeof(in));
    }
}

class Socket_client{
public:
    Socket_client():fd(-1){}
    ~Socket_client(){if(fd!=-1)close(fd);}
    bool _connect(std::string ser_name="",int port=50000)
    {
        std::string ser_port=std::to_string(port);
        addrinfo*result,hints;
        memset(&hints,0,sizeof(hints));
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype=SOCK_STREAM;
        hints.ai_protocol=0;
        hints.ai_flags=AI_NUMERICSERV;
        if(ser_name.size()==0)getaddrinfo(nullptr,ser_port.c_str(),&hints,&result);
        else getaddrinfo(ser_name.c_str(),ser_port.c_str(),&hints,&result);
        int ret=1;
        for(auto rp=result;rp!=nullptr;rp=rp->ai_next)
        {
            if((fd=socket(rp->ai_family,rp->ai_socktype|SOCK_CLOEXEC,rp->ai_protocol))==-1)continue;
            if(connect(fd,rp->ai_addr,rp->ai_addrlen)==0)
                break;
            close(fd);
            if(rp->ai_next==nullptr)ret=0;
        }
        freeaddrinfo(result);
        return ret;
    }
    int _fd()
    {
        return fd;
    }
private:
    int fd;
};

#endif
#ifndef chat_Server_Save_redis
#define chat_Server_Save_redis

#include<hiredis/hiredis.h>
#include<memory>
#include<string>
#include<vector>
#include<iostream>

using std::string;
class redis_res{
public:
    redis_res(redisReply*input=nullptr):res(input,freeReplyObject){}
    bool exist()
    {
        if(res==nullptr)return 0;
        return 1;
    }
    int type()
    {   if(res!=nullptr)
            return res.get()->type;
        else return 0;
    }
    bool type_error()
    {
        return error;
    }
    bool isnil()
    {  
        if(res==nullptr)return 0;
        if(res.get()->type==REDIS_REPLY_NIL)return 1;
        else return 0;
    }
    string getstring()
    {
        if(res==nullptr)return "";
        if(res.get()->type==REDIS_REPLY_STRING)
        {
            error=0;
            return string(res.get()->str,res.get()->len);
        }
        else
        {
            error=1;
            return "";
        }
    }
    std::vector<string> getarray()
    {
        std::vector<string> tmp;
        if(res==nullptr)return tmp;
        if(res.get()->type==REDIS_REPLY_ARRAY)
        {
            for(size_t c=0;c<res.get()->elements;c++)
            {
                tmp.emplace_back(res.get()->element[c]->str,res.get()->element[c]->len);
            }
            error=0;
            return std::move(tmp);
        }
        else 
        {
            error=1;
            return tmp;
        }
    }
    long long getinteger()
    {
        if(res==nullptr)return 0;
        if(res.get()->type==REDIS_REPLY_INTEGER)
        {
            error=0;
            return res.get()->integer;
        }
        else 
        {
            error=1;
            return 0;
        }
    }
    string getstatus()
    {
        if(res==nullptr)return "";
        if(res.get()->type==REDIS_REPLY_STATUS)
        {
            error=0;
            return res.get()->str;
        }
        else
        {
            error=1;
            return "";
        }
    }
    string geterror()
    {
        if(res==nullptr)return "";
        if(res.get()->type==REDIS_REPLY_ERROR)
        {
            error=0;
            return res.get()->str;
        }
        else
        {
            error=1;
            return "";
        }
    }
private:
    int error=0;
    std::unique_ptr<redisReply,void(*)(void*)> res;//redis?/void?
};
class redis_text{
public:
    redis_text(const char*_ip="127.0.0.1",int _port=6379):ip(_ip),port(_port),text(redisConnect(_ip,_port),redisFree)
    {
        if(text!=nullptr&&text.get()->err)
        {
            std::cerr<<text.get()->errstr<<'\n';
            text=nullptr;
        }
    }
    redis_res command(const std::vector<string>&argvs)
    {
        if(text==nullptr||text.get()->err)
        {
            reconnect();
            return nullptr;
        }
        const char* tmp[argvs.size()];
        size_t tmp2[argvs.size()];
        for(int c=0;c<argvs.size();c++)
        {
            tmp[c]=argvs[c].c_str();
            tmp2[c]=argvs[c].size();
        }
        redisReply*reply;
        if((reply=(redisReply*)redisCommandArgv(text.get(),argvs.size(),tmp,tmp2))==nullptr)
        {
            std::cerr<<text.get()->errstr<<'\n';
            reconnect();
            return nullptr;
        }
        return reply;
    }
    bool isconnect()
    {
        if(text==nullptr||text.get()->err)
        {
            return reconnect();
        }
        return 1;
    }
private:
    bool reconnect()
    {
        *this=redis_text(ip.c_str(),port);
        if(text==nullptr)return 0;
        return 1;
    }
private:
    string ip;
    int port;
    std::unique_ptr<redisContext,void(*)(redisContext*)> text;
};

#endif
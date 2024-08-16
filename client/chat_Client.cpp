#include"chat_Client.hpp"
int main(int argc,char**argv)
{
    if(argc!=2)
    {
        std::cerr<<"请输入服务器IP";
        return 1;
    }
    Client cli;
    cli.start(argv[1]);
}
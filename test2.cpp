#include<tuple>
#include<vector>
#include<iostream>
#include<map>
#include<memory>
#include<string.h>
#include"server/chat_Server_Save.hpp"
#include<openssl/sha.h>
using std::string;
void mysha(string &str){
    unsigned char md[33]={0};
    SHA256(reinterpret_cast<const unsigned char *>(str.c_str()),str.size(),md);
    str=reinterpret_cast<char *>(md);
}
int main()
{
    string tmp;
    std::cin>>tmp;
    mysha(tmp);
    std::cout<<tmp;
}
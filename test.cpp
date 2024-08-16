#include<tuple>
#include<vector>
#include<iostream>
#include<map>
#include<memory>
#include<string.h>
#include"server/chat_Server_Save.hpp"
using std::string;
int main()
{
    mysql_table a;
    auto b=a.s_f_wh_or("uid,fri_uid,file,time,file_name","u_files",
                                "((uid=1 and fri_uid=2)or(uid=2 and fri_uid=1))and file_name=aproto","time");;
    auto c=a.s_f_wh_or("uid,fri_uid,file,time,file_name","u_files",
                                "((uid=1 and fri_uid=2)or(uid=2 and fri_uid=1))and file_name=aproto","time");
    std::cerr<<"err"<<a.error_what();
    

}
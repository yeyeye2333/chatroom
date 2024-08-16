#ifndef chat_Server_Save
#define chat_Server_Save

#include"chat_Server_Save_mysql.hpp"
// #include"chat_Server_Save_redis.hpp"
#include<vector>
#include<string>
#include<mutex>
#define maxu_messages 100
#define maxu_files 3
#define maxgroup 3
#define maxg_messages 1000
#define maxg_files 10
// #define maxfile 65535
extern char cur_time[20];

using std::string;

class Save{
public:
    Save():ulock(mtx,std::defer_lock)
    {
        save_mysql.c_val_eng("u_info","uid int not null auto_increment,\
                                        user_name char(30) not null,\
                                        password char(30) not null,\
                                        group_num int not null,\
                                        primary key(uid)");
        save_mysql.c_val_eng("u_relation","uid int not null,\
                                            fri_uid int not null,\
                                            blok bool not null,\
                                            mess_num tinyint not null,\
                                            file_num tinyint not null,\
                                            has_mess bool not null,\
                                            primary key(uid,fri_uid)");
        save_mysql.c_val_eng("u_request","send_uid int not null,\
                                            recv_uid int not null,\
                                            primary key(send_uid,recv_uid)");
        save_mysql.c_val_eng("u_messages","uid int not null,\
                                            fri_uid int not null,\
                                            message varchar(1000) not null,\
                                            time datetime not null");
        save_mysql.c_val_eng("u_files","uid int not null,\
                                            fri_uid int not null,\
                                            file_len bigint not null,\
                                            time datetime not null,\
                                            file_name char(30) not null");//blob<=65535

        save_mysql.c_val_eng("g_members","gid int not null,\
                                            uid int not null,\
                                            is_manager bool not null,\
                                            primary key(gid,uid)");
        save_mysql.c_val_eng("g_info","gid int not null auto_increment,\
                                        group_master int not null,\
                                        group_name char(30) not null,\
                                        mess_num smallint not null,\
                                        file_num smallint not null,\
                                        primary key(gid)");        
        save_mysql.c_val_eng("g_request","send_uid int not null,\
                                            recv_gid int not null,\
                                            primary key(send_uid,recv_gid)");
        save_mysql.c_val_eng("g_messages","gid int not null,\
                                            uid int not null,\
                                            message varchar(1000) not null,\
                                            time datetime not null");   
        save_mysql.c_val_eng("g_files","gid int not null,\
                                        uid int not null,\
                                        file_len bigint not null,\
                                        time datetime not null,\
                                        file_name char(30) not null");                                                                                                                                  
    }
    
    string mysql_err()
    {
        return save_mysql.error_what();
    }
    bool mysql_iserr()
    {
        return save_mysql.iserr();
    }

    bool has_mess(int uid,int fri_uid)
    {
        return update_u_relation(std::to_string(uid),std::to_string(fri_uid),"","","","1");
    }
    std::vector<string> search_mess(int uid)
    {
        std::vector<string> res;
        auto tmp=save_mysql.s_f_wh_or("uid,fri_uid,blok,mess_num,file_num,has_mess","u_relation",
                            "fri_uid="+std::to_string(uid)+" and has_mess=1");
        update_u_relation("",std::to_string(uid),"","","","0");
        for(auto tmp2=tmp.getrow();tmp2.size();tmp2=tmp.getrow())
        {
            res.push_back(tmp2[0]);
        }
        return res;
    }
    bool fri_confirm(int uid,int fri_uid)
    {
        if(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).row_num()!=0)return 1;
        return 0;
    }
    bool g_confirm(int uid,int gid)
    {
        if(select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).row_num()!=0)return 1;
        return 0;
    }
    std::vector<int> g_manager(int gid)
    {
        std::vector<int> res;
        auto tmp=select_g_memebers_gid(std::to_string(gid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            if(tmp2[2]=="1")res.push_back(std::stoi(tmp2[1]));
        }
        return res;
    }
    std::vector<int> g_members(int gid)
    {
        std::vector<int> res;
        auto tmp=select_g_memebers_gid(std::to_string(gid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res.push_back(std::stoi(tmp2[1]));
        }
        return res;
    }

    bool login(int uid,string password)
    {
        auto tmp=select_u_info(std::to_string(uid));
        if(tmp.row_num()==0)return 0;
        auto row=tmp.getrow(3);
        if(row[0]==std::to_string(uid)&&row[2]==password)return 1;
        else return 0;
    }
    int signup(string user_name,string password)
    {
        int tmp;
        ulock.lock();
        if(insert_u_info(password,user_name)==0)tmp=0;
        else tmp=save_mysql.last_insert_id();
        ulock.unlock();
        return tmp;
    }
    bool logout(int uid,const string& password)
    {
        if(login(uid,password)==0)return 0;
        save_mysql.begin();
        bool b1=delete_u_info(std::to_string(uid));
        bool b2=delete_u_relation(std::to_string(uid));
        bool b3=delete_u_request(std::to_string(uid));
        bool b4=delete_u_messages_num(std::to_string(uid));
        bool b5=delete_u_files_num(std::to_string(uid));
        if(b1&b2&b3&b4&b5)save_mysql.commit();
        else save_mysql.rollback();
        return b1&b2&b3&b4&b5;
    }
    std::vector<std::vector<std::string>> u_search(int uid)
    {
        std::vector<std::string> res1;
        std::vector<std::string> res2;
        auto tmp=save_mysql.s_f_wh_or("fri_uid,user_name","u_relation,u_info",
                                    "fri_uid=u_info.uid and u_relation.uid="+std::to_string(uid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res1.push_back(tmp2[0]);
            res2.push_back(tmp2[1]);
        }
        return {res1,res2};
    }
    bool u_request(int send_uid,int recv_uid)
    {
        if(send_uid==recv_uid||select_u_info(std::to_string(recv_uid)).row_num()==0||
            select_u_relation(std::to_string(send_uid),std::to_string(recv_uid)).row_num()!=0)return 0;
        return insert_u_request(std::to_string(send_uid),std::to_string(recv_uid));
    }
    bool u_add(int uid,int fri_uid)
    {
        if(select_u_request(std::to_string(uid),std::to_string(fri_uid)).row_num()!=0)
        {
            save_mysql.begin();
            auto b1=delete_u_request(std::to_string(fri_uid),std::to_string(uid));
            auto b2=insert_u_relation(std::to_string(uid),std::to_string(fri_uid));
            auto b3=insert_u_relation(std::to_string(fri_uid),std::to_string(uid));
            if(b1&b2&b3)save_mysql.commit();
            else save_mysql.rollback();
            return b1&b2&b3;
        }
        else return 0;
    }
    std::vector<std::vector<string>> u_listreq(int uid)
    {
        std::vector<string> res1;
        std::vector<string> res2;
        {auto tmp=select_u_request(std::to_string(uid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res1.push_back(tmp2[0]);
        }}
        {auto tmp=select_u_info(res1);
        res1.clear();
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow()){
            res1.push_back(tmp2[0]);
            res2.push_back(tmp2[1]);
        }}
        return {res1,res2};
    }
    bool u_del(int uid,int fri_uid)
    {
        save_mysql.begin();
        auto b1=delete_u_relation(std::to_string(uid),std::to_string(fri_uid));
        auto b2=delete_u_relation(std::to_string(fri_uid),std::to_string(uid));
        auto b3=delete_u_files_num(std::to_string(uid),std::to_string(fri_uid));
        auto b4=delete_u_files_num(std::to_string(fri_uid),std::to_string(uid));
        auto b5=delete_u_messages_num(std::to_string(uid),std::to_string(fri_uid));
        auto b6=delete_u_messages_num(std::to_string(fri_uid),std::to_string(uid));
        if(b1&b2&b3&b4&b5&b6)save_mysql.commit();
        else save_mysql.rollback();
        return b1&b2&b3&b4&b5&b6;
    }
    bool u_blok(int uid,int fri_uid)
    {
        if(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).row_num()==0)return false;
        return update_u_relation(std::to_string(uid),std::to_string(fri_uid),"1");
    }
    bool u_unblok(int uid,int fri_uid)
    {
        if(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).row_num()==0)return false;
        return update_u_relation(std::to_string(uid),std::to_string(fri_uid),"0");
    }
    bool u_message(int uid,int fri_uid,string message)
    {
        if(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(3)[2]=="1"||
        select_u_relation(std::to_string(fri_uid),std::to_string(uid)).getrow(3)[2]=="1")return 0;
        bool b1=1,b2=1,b3=1,b4=1;
        save_mysql.begin();
        if((b1=insert_u_messages(std::to_string(uid),std::to_string(fri_uid),message,cur_time))==1)
        {
            b2=update_u_relation(std::to_string(uid),std::to_string(fri_uid),"","mess_num+1");
            if(std::stoi(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(4)[3])>maxu_messages)
            {
                if((b3=delete_u_messages_num(std::to_string(uid),std::to_string(fri_uid),10))==1)
                    b4=update_u_relation(std::to_string(uid),std::to_string(fri_uid),"","mess_num-10");
            }
        }
        if(b1&b2&b3&b4)save_mysql.commit();
        else{
            save_mysql.rollback();
        }
        return b1&b2&b3&b4;
    }
    bool u_file(int uid,int fri_uid,long file_len,string file_name)
    {
        if(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(3)[2]=="1"||
        select_u_relation(std::to_string(fri_uid),std::to_string(uid)).getrow(3)[2]=="1")return 0;
        bool b1=1,b2=1,b3=1,b4=1;
        save_mysql.begin();
        if((b1=insert_u_files(std::to_string(uid),std::to_string(fri_uid),std::to_string(file_len),cur_time,file_name))==1)
        {
            b2=update_u_relation(std::to_string(uid),std::to_string(fri_uid),"","","file_num+1");
            if(std::stoi(select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(5)[4])>maxu_files)
            {
                if((b3=delete_u_messages_num(std::to_string(uid),std::to_string(fri_uid),1))==1)
                    b4=update_u_relation(std::to_string(uid),std::to_string(fri_uid),"","","file_num-10");
            }
        }
        if(b1&b2&b3&b4)save_mysql.commit();
        else save_mysql.rollback();
        return b1&b2&b3&b4;
    }
    std::vector<std::vector<string>> u_m_history(int uid,int fri_uid)
    {
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(1);
        auto res=select_u_messages(std::to_string(uid),std::to_string(fri_uid));
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[2]);
            tmp2.push_back(tmp[3]);
            tmp3.push_back(tmp[0]);
        }
        return {tmp3,tmp1,tmp2};
    }
    std::vector<std::vector<string>> u_f_history0(int uid,int fri_uid)
    {
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(1);
        auto res=select_u_files(std::to_string(uid),std::to_string(fri_uid));
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[3]);
            tmp2.push_back(tmp[2]);
            tmp3.push_back(tmp[0]);
        }
        return {tmp3,tmp1,tmp2};
    }
    std::vector<std::vector<string>> u_f_history1(int uid,int fri_uid,string file_name)
    {
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        std::vector<string> tmp4;
        select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(1);
        auto res=select_u_files(std::to_string(uid),std::to_string(fri_uid),file_name);
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[2]);
            tmp2.push_back(tmp[3]);
            tmp3.push_back(tmp[4]);
            tmp4.push_back(tmp[0]);
        }
        return {tmp4,tmp3,tmp1,tmp2};
    }
    int g_create(int uid,string g_name)
    {
        if(std::stoi(select_u_info(std::to_string(uid)).getrow(4)[3])>=maxgroup)return 0;
        save_mysql.begin();
        ulock.lock();
        auto b1=insert_g_info(std::to_string(uid),g_name);
        auto last_id=save_mysql.last_insert_id();
        auto b2=insert_g_memebers(std::to_string(last_id),std::to_string(uid),"1");
        ulock.unlock();
        auto b3=update_u_info(std::to_string(uid),"group_num+1");
        if(b1&b2&b3){
            save_mysql.commit();
            return last_id;
        }
        else{
            save_mysql.rollback();
            return 0;
        }
    }
    bool g_disban(int uid,int gid)
    {
        if(std::stoi(select_g_info(std::to_string(gid)).getrow(2)[1])!=uid)return 0;
        save_mysql.begin();
        auto b1=update_u_info(std::to_string(uid),"group_num-1");
        auto b2=delete_g_info(std::to_string(gid));
        auto b3=delete_g_members_gid(std::to_string(gid));
        auto b4=delete_g_request_gid(std::to_string(gid));
        auto b5=delete_g_messages(std::to_string(gid));
        auto b6=delete_g_files(std::to_string(gid));
        if(b1&b2&b3&b4&b5&b6)save_mysql.commit();
        else save_mysql.rollback();
        return b1&b2&b3&b4&b5&b6;
    }
    bool g_request(int send_uid,int recv_gid)
    {
        if(select_g_info(std::to_string(recv_gid)).row_num()!=0||
            select_g_memebers_gid(std::to_string(recv_gid),std::to_string(send_uid)).row_num()==0)
            return insert_g_request(std::to_string(send_uid),std::to_string(recv_gid));
        else return 0;
    }
    bool g_add(int manager,int gid,int uid)
    {
        if(select_g_request(std::to_string(gid),std::to_string(uid)).row_num()==0||
            std::stoi(select_g_memebers_gid(std::to_string(gid),std::to_string(manager)).getrow(3)[2])==0)
            return 0;
        else
        {
            save_mysql.begin();
            auto b1=delete_g_request_gid(std::to_string(gid),std::to_string(uid));
            auto b2=insert_g_memebers(std::to_string(gid),std::to_string(uid),"0");
            if(b1&b2)save_mysql.commit();
            else save_mysql.rollback();
            return b1&b2;
        }
    }
    bool g_del(int manager,int gid,int uid)
    {   
        if(std::stoi(select_g_memebers_gid(std::to_string(gid),std::to_string(manager)).getrow(3)[2])==0||
            std::stoi(select_g_info(std::to_string(gid)).getrow(2)[1])==uid||
            std::stoi(select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3)[2])==1)
        { 
            return 0;
        }
        else return delete_g_members_uid(std::to_string(uid));
    }
    std::vector<std::vector<string>> g_listreq(int manager,int gid)
    {
        std::vector<string> res1;
        std::vector<string> res2;
        if(std::stoi(select_g_memebers_gid(std::to_string(gid),std::to_string(manager)).getrow(3)[2])==0)return {res1,res2};
        {auto tmp=select_g_request(std::to_string(gid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res1.push_back(tmp2[0]);
        }}
        {auto tmp= select_u_info(res1);
        res1.clear();
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow()){
            res1.push_back(tmp2[0]);
            res2.push_back(tmp2[1]);
        }}
        return {res1,res2};
    }
    std::vector<std::vector<string>> g_search(int uid)
    {
        std::vector<string> res1;
        std::vector<string> res2;
        auto tmp=save_mysql.s_f_wh_or("g_members.gid,group_name","g_members,g_info",
                                    "g_members.gid=g_info.gid and g_members.uid="+std::to_string(uid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res1.push_back(tmp2[0]);
            res2.push_back(tmp2[1]);
        }
        return {res1,res2};
    }
    bool g_message(int uid,int gid,string message)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        bool b1=1,b2=1,b3=1,b4=1;
        save_mysql.begin();
        if((b1=insert_g_messages(std::to_string(gid),std::to_string(uid),message,cur_time))==1)
        {
            b2=update_g_info(std::to_string(gid),"mess_num+1");
            if(std::stoi(select_g_info(std::to_string(gid)).getrow(4)[3])>maxg_messages)
            {
                if((b3=delete_g_messages(std::to_string(gid),50))==1)
                    b4=update_g_info(std::to_string(gid),"mess_num-50");
            }
        }
        if(b1&b2&b3&b4)save_mysql.commit();
        else{
            save_mysql.rollback();
        }
        return b1&b2&b3&b4;
    }
    bool g_file(int uid,int gid,long file_len,string file_name)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        bool b1=1,b2=1,b3=1,b4=1;
        save_mysql.begin();
        if((b1=insert_g_files(std::to_string(gid),std::to_string(uid),std::to_string(file_len),cur_time,file_name))==1)
        {
            b2=update_g_info(std::to_string(gid),"","file_num+1");
            if(std::stoi(select_g_info(std::to_string(gid)).getrow(5)[4])>maxg_files)
            {
                if((b3=delete_g_files(std::to_string(gid),2))==1)
                    b4=update_g_info(std::to_string(gid),"","file_num-2");
            }
        }
        if(b1&b2&b3&b4)save_mysql.commit();
        else save_mysql.rollback();
        return b1&b2&b3&b4;
    }
    bool g_quit(int uid,int gid)
    {
        if(std::stoi(select_g_info(std::to_string(gid)).getrow(2)[1])!=uid)
        {
            return delete_g_members_gid(std::to_string(gid),std::to_string(uid));
        }
        else return g_disban(uid,gid);
    }
    std::vector<std::vector<string>> g_members(int uid,int gid)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        std::vector<string> res1;
        std::vector<string> res2;
        std::vector<string> res3;
        auto tmp=select_g_memebers_gid(std::to_string(gid));
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow())
        {
            res1.push_back(tmp2[1]);
            res2.push_back(tmp2[2]);
        }
        tmp= select_u_info(res1);
        for(auto tmp2=tmp.getrow();tmp2.size()!=0;tmp2=tmp.getrow()){
            res3.push_back(tmp2[1]);
        }
        return {res1,res2,res3};
    }
    bool g_addmanager(int uid,int gid,int manager)
    {
        if(std::stoi(select_g_info(std::to_string(gid)).getrow(2)[1])==uid&&
            select_g_memebers_gid(std::to_string(gid),std::to_string(manager)).row_num()!=0)
        {
            return update_g_members(std::to_string(gid),std::to_string(manager),"1");
        }
        else return 0;
    }
    bool g_delmanager(int uid,int gid,int manager)
    {
        if(std::stoi(select_g_info(std::to_string(gid)).getrow(2)[1])==uid&&
            select_g_memebers_gid(std::to_string(gid),std::to_string(manager)).row_num()!=0)
        {
           return update_g_members(std::to_string(gid),std::to_string(manager),"0");
        }
        else return 0;
    }
    std::vector<std::vector<string>> g_m_history(int uid,int gid)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        if(select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).row_num()==0)return {tmp1,tmp2,tmp3};
        auto res=select_g_messages(std::to_string(gid));
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[2]);
            tmp2.push_back(tmp[3]);
            tmp3.push_back(tmp[1]);
        }
        return {tmp3,tmp1,tmp2};
    }
    std::vector<std::vector<string>> g_f_history0(int uid,int gid)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        if(select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).row_num()==0)return {tmp1,tmp2,tmp3};
        auto res=select_g_files(std::to_string(gid));
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[3]);
            tmp2.push_back(tmp[2]);
            tmp3.push_back(tmp[1]);
        }
        return {tmp3,tmp2,tmp1};
    }
    std::vector<std::vector<string>> g_f_history1(int uid,int gid,string file_name)
    {
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
        std::vector<string> tmp1;
        std::vector<string> tmp2;
        std::vector<string> tmp3;
        std::vector<string> tmp4;
        if(select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).row_num()==0)return {tmp1,tmp2,tmp3,tmp4};
        auto res=select_g_files(std::to_string(gid),file_name);
        for(auto tmp=res.getrow();tmp.size()!=0;tmp=res.getrow())
        {
            tmp1.push_back(tmp[2]);
            tmp2.push_back(tmp[3]);
            tmp3.push_back(tmp[4]);
            tmp4.push_back(tmp[1]);
        }
        return {tmp4,tmp3,tmp1,tmp2};
    }

    void is_gmember(int uid,int gid){
        select_g_memebers_gid(std::to_string(gid),std::to_string(uid)).getrow(3);
    }
    void is_fri(int uid,int fri_uid){
        select_u_relation(std::to_string(uid),std::to_string(fri_uid)).getrow(3);
    }
private:
    //user
    mysql_res select_u_info(const string &uid)
    {
        return save_mysql.s_f_wh_or("uid,user_name,password,group_num","u_info","uid="+uid);
    }
    mysql_res select_u_info(const std::vector<string> &uids)
    {
        string in_sql;
        for(auto &tmp:uids){
            in_sql+=tmp+",";
        }
        if(in_sql.size()>0)in_sql.pop_back();
        return save_mysql.s_f_wh_or("uid,user_name,password,group_num","u_info","uid in("+in_sql+")");
    }
    mysql_res select_u_relation(const string &uid="",const string&fri_uid="")
    {
        if(fri_uid.size()==0)return save_mysql.s_f_wh_or("uid,fri_uid,blok,mess_num,file_num,has_mess","u_relation","uid="+uid);
        else if(uid.size()==0)return save_mysql.s_f_wh_or("uid,fri_uid,blok,mess_num,file_num,has_mess","u_relation","fri_uid="+fri_uid);
        else return save_mysql.s_f_wh_or("uid,fri_uid,blok,mess_num,file_num,has_mess","u_relation",
                                "uid="+uid+" and fri_uid="+fri_uid);
    }
    mysql_res select_u_request(const string &recv_uid,const string&send_uid="")
    {
        if(send_uid.size()==0)return save_mysql.s_f_wh_or("send_uid,recv_uid","u_request","recv_uid="+recv_uid);
        else return save_mysql.s_f_wh_or("send_uid,recv_uid","u_request","recv_uid="+recv_uid+" and send_uid="+send_uid);
    }
    mysql_res select_u_messages(const string &uid,const string&fri_uid)
    {
        return save_mysql.s_f_wh_or("uid,fri_uid,message,time","u_messages",
                                "(uid="+uid+" and fri_uid="+fri_uid+")or(uid="+fri_uid+" and fri_uid="+uid+")",
                                "time");
    }
    mysql_res select_u_files(const string &uid,const string&fri_uid)
    {
        return save_mysql.s_f_wh_or("uid,fri_uid,time,file_name","u_files",
                "(uid="+uid+" and fri_uid="+fri_uid+")or(uid="+fri_uid+" and fri_uid="+uid+")","time");
    }
    mysql_res select_u_files(const string &uid,const string&fri_uid,string&file_name)
    {
        save_mysql.escape_sql(file_name);
        return save_mysql.s_f_wh_or("uid,fri_uid,file_len,time,file_name","u_files",
                "((uid="+uid+" and fri_uid="+fri_uid+")or(uid="+fri_uid+" and fri_uid="+uid+"))and file_name=\'"+file_name+"\'","time");
    }

    bool insert_u_info(string &password,string &user_name,const string &group_num="0")
    {
        save_mysql.escape_sql(password);
        save_mysql.escape_sql(user_name);
        return save_mysql.i_type_val("u_info","user_name,password,group_num",
                                "\'"+user_name+"\',\'"+password+"\',"+group_num);
    }
    bool insert_u_relation(const string &uid,const string &fri_uid,const string &blok="0",const string &mess_num="0",
                        const string &file_num="0",const string &has_mess="0")
    {
        return save_mysql.i_type_val("u_relation","uid,fri_uid,blok,mess_num,file_num,has_mess",
                                uid+","+fri_uid+","+blok+","+mess_num+","+file_num+","+has_mess);
    }
    bool insert_u_request(const string &send_uid,const string &recv_uid)
    {
        return save_mysql.i_type_val("u_request","send_uid,recv_uid",
                                send_uid+","+recv_uid);
    }
    bool insert_u_messages(const string &uid,const string &fri_uid,string &message,const string &time)
    {
        save_mysql.escape_sql(message);
        return save_mysql.i_type_val("u_messages","uid,fri_uid,message,time",
                                uid+","+fri_uid+",\'"+message+"\',\'"+time+"\'");
    }
    bool insert_u_files(const string &uid,const string &fri_uid,const string &file_len,const string &time,string&file_name)
    {
        save_mysql.escape_sql(file_name);
        return save_mysql.i_type_val("u_files","uid,fri_uid,file_len,time,file_name",
                                uid+","+fri_uid+",\'"+file_len+"\',\'"+time+"\',\'"+file_name+"\'");
    }

    bool delete_u_info(const string&uid)
    {
        return save_mysql.d("u_info where uid="+uid);
    }
    bool delete_u_relation(const string&uid,const string&fri_uid="")
    {
        if(fri_uid.size()==0)return save_mysql.d("u_relation where uid="+uid+" or fri_uid="+uid);
        else return save_mysql.d("u_relation where uid="+uid+" and fri_uid="+fri_uid);
    }
    bool delete_u_request(const string&send_uid,const string&recv_uid="")
    {
        if(recv_uid.size()==0)return save_mysql.d("u_request where send_uid="+send_uid+" or recv_uid="+send_uid);
        else return save_mysql.d("u_request where send_uid="+send_uid+" and recv_uid="+recv_uid);
    }
    bool delete_u_messages_num(const string&uid,const string&fri_uid="",int num=1000)
    {
        if(fri_uid.size()==0)return save_mysql.d("u_messages where uid="+uid+" or fri_uid="+uid);
        else return save_mysql.d("u_messages where uid="+uid+" and fri_uid="+fri_uid+" order by time limit "+std::to_string(num));
    }
    bool delete_u_files_num(const string&uid,const string&fri_uid="",int num=1000)
    {
        if(fri_uid.size()==0)return save_mysql.d("u_files where uid="+uid+" or fri_uid="+uid);
        else return save_mysql.d("u_files where uid="+uid+" and fri_uid="+fri_uid+" order by time limit "+std::to_string(num));
    }

    bool update_u_info(const string&uid,const string&group_num="",const string&user_name="",const string&password="")
    {
        string res;
        bool is=0;
        if(group_num.size()!=0)
        {
            res+="group_num="+group_num;
            is=1;
        }
        if(user_name.size()!=0)
        {
            if(is==1)res+=",";
            res+="user_name=\'"+user_name+"\'";
            is=1;
        }
        if(password.size()!=0)
        {
            if(is==1)res+=",";
            res+="password=\'"+password+"\'";
        }
        return save_mysql.up_set_wh("u_info",res,"uid="+uid);
    }
    bool update_u_relation(const string& uid="",const string& fri_uid="",const string&blok="",const string&mess_num="",const string&file_num="",const string&has_mess="")
    {
        string res;
        bool is=0;
        if (blok.size()!=0)
        {
            res += "blok=" + blok;
            is=1;
        }
        if (mess_num.size()!=0)
        {
            if(is==1)res+=",";
            res += "mess_num=" + mess_num;
            is=1;
        }
        if (file_num.size()!=0)
        {
            if(is==1)res+=",";
            res += "file_num=" + file_num;
            is=1;
        }
        if (has_mess.size()!=0)
        {
            if(is==1)res+=",";
            res += "has_mess=" + has_mess;
        }
        if(uid.size()==0)return save_mysql.up_set_wh("u_relation",res,"fri_uid="+fri_uid);
        return save_mysql.up_set_wh("u_relation", res,"uid="+uid+" and fri_uid="+fri_uid);
    }

    //group
    mysql_res select_g_memebers_gid(const string &gid,const string&uid="")
    {
        if(uid.size()==0)return save_mysql.s_f_wh_or("gid,uid,is_manager","g_members","gid="+gid);
        else return save_mysql.s_f_wh_or("gid,uid,is_manager","g_members","gid="+gid+" and uid="+uid);
    }
    mysql_res select_g_memebers_uid(const string &uid)
    {
        return save_mysql.s_f_wh_or("gid,uid,is_manager","g_members","uid="+uid);
    }
    mysql_res select_g_info(const string &gid)
    {
        return save_mysql.s_f_wh_or("gid,group_master,group_name,mess_num,file_num","g_info","gid="+gid);
    }
    mysql_res select_g_request(const string &recv_gid,const string&send_uid="")
    {
        if(send_uid.size()==0)return save_mysql.s_f_wh_or("send_uid,recv_gid","g_request","recv_gid="+recv_gid);
        else return save_mysql.s_f_wh_or("send_uid,recv_gid","g_request","recv_gid="+recv_gid+" and send_uid="+send_uid);
    }
    mysql_res select_g_messages(const string &gid)
    {
        return save_mysql.s_f_wh_or("gid,uid,message,time","g_messages","gid="+gid,"time");
    }
    mysql_res select_g_files(const string &gid)
    {

        return save_mysql.s_f_wh_or("gid,uid,time,file_name","g_files","gid="+gid,"time");
    }
    mysql_res select_g_files(const string &gid,string& file_name)
    {
        save_mysql.escape_sql(file_name);
        return save_mysql.s_f_wh_or("gid,uid,file_len,time,file_name","g_files","gid="+gid+" and file_name=\'"+file_name+"\'","time");
    }

    bool insert_g_memebers(const string &gid,const string &uid,const string &is_manager)
    {
        return save_mysql.i_type_val("g_members","gid,uid,is_manager",
                                    gid+","+uid+","+is_manager);
    }
    bool insert_g_info(const string &group_master,string &group_name,const string &mess_num="0",const string &file_num="0")
    {
        save_mysql.escape_sql(group_name);
        return save_mysql.i_type_val("g_info","group_master,group_name,mess_num,file_num",
                                    group_master+",\'"+group_name+"\',"+mess_num+","+file_num);
    }
    bool insert_g_request(const string &send_uid,const string &recv_gid)
    {
        return save_mysql.i_type_val("g_request","send_uid,recv_gid",
                                    send_uid+","+recv_gid);
    }
    bool insert_g_messages(const string &gid,const string &uid,string &message,const string &time)
    {
        save_mysql.escape_sql(message);
        return save_mysql.i_type_val("g_messages","gid,uid,message,time",
                                    gid+","+uid+",\'"+message+"\',\'"+time+"\'");
    }
    bool insert_g_files(const string &gid,const string &uid,const string &file_len,const string &time,string &file_name)
    {
        save_mysql.escape_sql(file_name);
        return save_mysql.i_type_val("g_files","gid,uid,file_len,time,file_name",
                                    gid+","+uid+",\'"+file_len+"\',\'"+time+"\',\'"+file_name+"\'");
    }

    bool delete_g_members_gid(const string &gid,const string &uid="")
    {
        if(uid.size()==0)return save_mysql.d("g_members where gid="+gid);
        else return save_mysql.d("g_members where gid="+gid+" and uid="+uid);
    }
    bool delete_g_members_uid(const string &uid)
    {
        return save_mysql.d("g_members where uid="+uid);
    }
    bool delete_g_info(const string &gid)
    {
        return save_mysql.d("g_info where gid="+gid);
    }
    bool delete_g_request_gid(const string &recv_gid,const string &send_uid="")
    {
        if(send_uid.size()==0)return save_mysql.d("g_request where recv_gid="+recv_gid);
        return save_mysql.d("g_request where recv_gid="+recv_gid+" and send_uid="+send_uid);
    }
    bool delete_g_request_uid(const string &send_uid)
    {
        return save_mysql.d("g_request where send_uid="+send_uid);
    }
    bool delete_g_messages(const string &gid,int num=-1)
    {
        if(num=-1)return save_mysql.d("g_messages where gid="+gid);
        else return save_mysql.d("g_messages where gid="+gid+" order by time limit "+std::to_string(num));
    }
    bool delete_g_files(const string &gid,int num=-1)
    {
        if(num=-1)return save_mysql.d("g_files where gid="+gid);
        else return save_mysql.d("g_files where gid="+gid+" order by time limit "+std::to_string(num));
    }

    bool update_g_members(const string&gid,const string&uid,const string&is_manager="")
    {
        string res;
        if (is_manager.size() != 0) res += "is_manager=" + is_manager;
        return save_mysql.up_set_wh("g_members", res, "gid=" + gid + " and uid=" + uid);
    }  
    bool update_g_info(const string& gid,const string&mess_num="",const string&file_num="")
    {
        string res;
        bool is=0;
        if (mess_num.size() != 0)
        {
            res += "mess_num=" + mess_num;
            is=1;
        }
        if (file_num.size() != 0)
        {
            if(is==1)res+=",";
            res += "file_num=" + file_num;
        }
        return save_mysql.up_set_wh("g_info",res,"gid=" + gid);
    }
private:
    mysql_table save_mysql;
    // redis_text save_redis;
    static std::mutex mtx;
    std::unique_lock<std::mutex> ulock;
};
std::mutex Save::mtx;
#endif

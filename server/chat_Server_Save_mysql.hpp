#ifndef chat_Server_Save_mysql
#define chat_Server_Save_mysql

#include</usr/include/mysql/mysql.h>
// #include<hiredis/hiredis.h>
#include<mutex>
#include<string>
#include<memory>
#include<vector>
#include<iostream>

using std::string;
using std::unique_ptr;

class select_err{
};

enum {se,in,de,up,cr,dr};

class mysql_res{
public:
    mysql_res(MYSQL_RES*input):res(input,mysql_free_result){}
    
    int col_num()
    {
        if(res==nullptr)return 0;
        else
        {
            return mysql_num_fields(res.get());
        }
    }
    int row_num()
    {
        if(res==nullptr)return 0;
        else
        {
            return mysql_num_rows(res.get());
        }
    }
    std::vector<string> getrow(int least=0)//小于least抛出错误
    {
        std::vector<string> tmp;
        if(res.get()==nullptr){
            return tmp;
        }
        int num=col_num();
        MYSQL_ROW row;
        if((row=mysql_fetch_row(res.get()))!=nullptr)
        {
            for(int c=0;c<num;c++)
            {
                tmp.push_back(row[c]);
            }
        }
        if(tmp.size()<least){
            std::cerr<<"select "<<tmp.size()<<":"<<least<<std::endl;
            throw select_err();
        }
        return std::move(tmp);
    }
    void release(){
        res.reset();
    }
private:
    unique_ptr<MYSQL_RES,void(*)(MYSQL_RES*)> res;
};

class mysql_table{
public:
    mysql_table(string _dbname="chatroom",string _user="root",string _passwd="123456",string _host="",int _port=0)
    {
        std::call_once(begin_flag,mysql_library_init,0,nullptr,nullptr);
        mysql_init(&db);
        mysql_real_connect(&db,_host.c_str(),_user.c_str(),_passwd.c_str(),_dbname.c_str(),_port,nullptr,0);
    }
    ~mysql_table()
    {
        mysql_close(&db);
        std::call_once(end_flag,mysql_library_end);
    }
    bool ping()
    {
        return !mysql_ping(&db);
    }
    
    int next_res(){
        return mysql_next_result(&db);
    }
    void escape_sql(string & target){
        char *to=new char[target.size()*2+1];
        mysql_real_escape_string(&db,to,target.c_str(),target.size());
        target=string(to);
        delete to;
    }

    mysql_res s_f_wh_or(string name,string from,string where="",string order="");
    bool i_type_val(string name,string type,string val);
    bool d(string name);
    bool up_set_wh(string name,string set,string wh);
    bool c_val_eng(string name,string val_pri,string engine="innodb");
    bool drop(string name);
    long last_insert_id()
    {
        return mysql_insert_id(&db);
    }
    bool begin()
    {
        return mysql_query(&db,"begin");
    }
    bool commit()
    {
        return mysql_query(&db,"commit");
    }
    bool rollback()
    {
        return mysql_query(&db,"rollback");
    }
    string error_what()
    {
        return mysql_error(&db);
    }
    bool iserr()
    {
        return mysql_errno(&db);
    }
private:
    string getsql(int flag,string a,string b="",string c="",string d="");
private:
    MYSQL db;
    static std::once_flag begin_flag;
    static std::once_flag end_flag;
};
std::once_flag mysql_table::begin_flag;
std::once_flag mysql_table::end_flag;


mysql_res mysql_table::s_f_wh_or(string col,string name,string where,string order)
{
    string sql=getsql(se,col,name,where,order);
    mysql_real_query(&db,sql.c_str(),sql.size());
    auto ret=mysql_store_result(&db);next_res();
    return ret;
}
bool mysql_table::i_type_val(string name,string type,string val)
{
    string sql=getsql(in,name,type,val);
    return !mysql_real_query(&db,sql.c_str(),sql.size());
}
bool mysql_table::d(string name)
{
    string sql=getsql(de,name);
    return !mysql_real_query(&db,sql.c_str(),sql.size());
}
bool mysql_table::up_set_wh(string name,string set,string wh)
{
    string sql=getsql(up,name,set,wh);
    return !mysql_real_query(&db,sql.c_str(),sql.size());
}
bool mysql_table::c_val_eng(string name,string val_pri,string engine)
{
    string sql=getsql(cr,name,val_pri,engine);
    return !mysql_real_query(&db,sql.c_str(),sql.size());
}
bool mysql_table::drop(string name)
{  
    string sql=getsql(dr,name);
    return !mysql_real_query(&db,sql.c_str(),sql.size());
}
string mysql_table::getsql(int flag,string a,string b,string c,string d)
{
    string sql;
    switch (flag)
    {
        case se:
            if(d.size()!=0)
                sql="select "+a+" from "+b+" where "+c+" order by "+d;
            else if(c.size()!=0)
                sql="select "+a+" from "+b+" where "+c;
            else 
                sql="select "+a+" from "+b;
            break;
        
        case in:
            sql="insert into "+a+"("+b+")values("+c+")";
            break;

        case de:
            sql="delete from "+a;
            break;

        case up:
            sql="update "+a+" set "+b+" where "+c;
            break;

        case cr:
            sql="create table if not exists "+a+"("+b+")engine="+c;
            break;

        case dr:
            sql="drop table "+a;
            break;

        default:
            break;
    }
    return std::move(sql);
}


#endif
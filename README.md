# chatroom    

采用c/s模式的多人在线聊天室  
- 使用 **线程池 + 非阻塞socket + epoll(LT均实现) + 事件处理(Reactor实现)** 的并发模型

## 框架
### 客户端
![client](https://github.com/yeyeye2333/chatroom/blob/master/IMG/client.png)
### 服务器
![server](https://github.com/yeyeye2333/chatroom/blob/master/IMG/server.png)

## 快速运行
### 本地构建
-  cmake版本需达3.26.5  

    (环境:arch)
    构建: cd chatroom  
         protoc -I=. --cpp_out=. ./chatroom.proto  
         cd client&&mkdir build&&cd build  
         cmake ..&&cmake --build .  
    运行: ./chatclient  
### docker运行
- 要使用文件相关功能需-v进行挂载

    拉取：docker pull yeyeye2333/chatclient  
    运行：docker参数-it，程序参数[ip] eg：docker run -it yeyeye2333/chatclient [ip]

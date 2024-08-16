# chatroom
    (环境:arch)
    构建: cd C-Work/mychatroom  
         protoc -I=. --cpp_out=. ./chatroom.proto  
         cd client&&mkdir build&&cd build  
         cmake ..&&cmake --build .  
    运行: ./chatclient  

或

    拉取：docker pull yeyeye2333/chatclient  
    运行：docker参数-it，程序参数[ip] eg：docker run -it yeyeye2333/chatclient [ip]

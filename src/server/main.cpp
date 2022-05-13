#include <iostream>
#include "chatserver.hpp"


using namespace std;

int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6098);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
    return 1;
}
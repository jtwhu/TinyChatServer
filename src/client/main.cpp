// stl
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <iostream>
#include <unordered_map>
// tcp
#include <unistd.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
// local files
#include "user.hpp"
#include "json.hpp"
#include "group.hpp"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};
// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 聊天主菜单页面
void MainMenu(int clientfd);
// 处理注册的响应逻辑
void DoRegResponse(json &responsejs);
// 处理登录响应的业务逻辑
void DoLoginResponse(json& responsejs);
// 接收消息的子线程
void ReadTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string GetCurrentTime();
// 显示当前登录成功用户的基本信息
void ShowCurrentUserData();

// "help" command handler
void Help(int fd=0, string str="");
// "chat" command handler
void Chat(int, string);
// "addfriend" command handler
void AddFriend(int, string);
// "creategroup" command handler
void CreateGroup(int, string);
// "addgroup" command handler
void AddGroup(int, string);
// "groupchat" command handler
void GroupChat(int, string);
// "loginout" command handler
void LoginOut(int, string);

// 系统支持的客户端命令提示
unordered_map<string, string> commandMap={
    {"help", "显示所有支持的指令，格式为help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友，格式chat:friendid"},
    {"creategroup","创建群组，格式createfroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式addgroup:groupid"},
    {"groupchat","群聊，格式为groupchat:groupid:message"},
    {"loginout","注销，格式loginout"}
};
// 系统支持的客户端命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap{
    {"help", Help},
    {"chat", Chat},
    {"addfriend", AddFriend},
    {"creategroup", CreateGroup},
    {"addgroup", AddGroup},
    {"groupchat", GroupChat},
    {"loginout", LoginOut}
};

// 聊天客户端程序实现，main线程用作发送主线程，子线程用作接收线程
int main(int argc, char* argv[]){
    if(argc < 3){
        cerr << "command invalid! example:./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd){
        cerr << "socket create error!" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));// 清空结构体

    server.sin_family = AF_INET;
    server.sin_port = htons(port);// 主机字节顺序转换为网络字节顺序
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if(-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in))){
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    std::thread ReadTask(ReadTaskHandler, clientfd);// pthread_create
    ReadTask.detach();                              // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for(;;){
        // 显示首页面菜单，登录、注册、退出
        cout << "=============================================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=============================================" << endl;
        cout << "choice:" << endl;
        int choice = 0;
        cin >> choice;
        cin.get(); // 读取掉缓冲区残留的回车

        switch (choice)
        {
        case 1:{// login业务
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get();// 读掉缓冲区中残留的回车
            cout << "userpassword:";
            cin.getline(pwd, 50);// 防止因为输入的字符串中有空格导致停止输入

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
            if(len == -1){
                cerr << "send login msg error:" << request << endl;
            }

            sem_wait(&rwsem);// 等待信号量，由子线程处理完登录的响应消息之后，通知这里

            if(g_isLoginSuccess){
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                MainMenu(clientfd);
            }
        }
        break;
        case 2:{// register业务
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
            if(len == -1){
                cerr << "send reg msg error" << request << endl;
            }

            sem_wait(&rwsem);// 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3:{// quit业务
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        break;
        default:
            cerr << "invalid input!" << endl;
            break;   
        } 
    }
    return 0;
}

// 接收消息的子线程
void ReadTaskHandler(int clientfd){
    for(;;){
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);// 进行阻塞

        if(-1 == len || 0 == len){
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype){
            cout << js["time"].get<string>() << "[" << js["id"] << "]:" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
                 continue;
        }

        if(GROUP_CHAT_MSG == msgtype){
            cout << "group message[" << js["groupid"] << "]:" << js["time"].get<string>() << "[" << js["id"] << js["name"].get<string>()
                 << "said:" << js["msg"].get<string>() << endl;
            continue;
        }

        if(LOGIN_MSG_ACK == msgtype){
            DoLoginResponse(js);// 处理登录响应的业务逻辑
            sem_post(&rwsem); // 通知主线程，登录结果处理完成
            continue;
        }

        if(REG_MSG_ACK == msgtype){
            DoRegResponse(js);
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
    }    
}

// 处理登录响应的业务逻辑
void DoLoginResponse(json& responsejs){
    if(0 != responsejs["errno"].get<int>()){// 登录失败
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }else{// 登录成功
        // 记录当前用户的id以及name
        g_currentUser.SetId(responsejs["id"].get<int>());
        g_currentUser.SetName(responsejs["name"]);

        // 记录当前用户的好友信息
        if(responsejs.contains("friends")){// 如果有好友字段
            // 初始化
            g_currentUserFriendList.clear();
            vector<string> localFriendsVec = responsejs["friends"];// 直接与stl容器进行转换
            for(string &str : localFriendsVec){
                json js = json::parse(str);
                User user;
                user.SetId(js["id"].get<int>());
                user.SetName(js["name"]);
                user.SetStat(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群列表消息
        if(responsejs.contains("groups")){
            // 初始化
            g_currentUserGroupList.clear();

            vector<string> localGroupVec = responsejs["groups"];
            for(string &groupstr : localGroupVec){
                json grpjs = json::parse(groupstr);
                Group group;
                // 群组基本信息
                group.SetId(grpjs["id"].get<int>());
                group.SetName(grpjs["groupname"]);
                group.SetDesc(grpjs["groupdesc"]);

                // 群组中其他用户的信息
                vector<string> localGroupUsersVec = grpjs["users"];
                for(string &userstr : localGroupVec){
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.SetId(js["id"].get<int>());
                    user.SetName(js["name"]);
                    user.SetStat(js["state"]);
                    user.SetRole(js["role"]);
                    group.GetUsers().push_back(user);// 这个地方有点好玩，因为是返回引用，所以可以直接打破了权限设定了
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        ShowCurrentUserData();
        
        // 显示当前用户的离线消息，包括一对一聊天消息和群组消息
        if(responsejs.contains("offlinemsg")){
            vector<string> localOfflineMsg = responsejs["offlinemsg"];
            for(string& str : localOfflineMsg){
                json js = json::parse(str);
                if(ONE_CHAT_MSG == js["msgid"].get<int>()){
                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                         << " said :" << js["msg"].get<string>() << endl;
                }else{
                    cout << "group message[" << js["groupid"] << "]:" << js["time"].get<string>() << "[" << js["id"] << "]"
                         << js["name"].get<string>() << "said:" << js["msg"].get<string>() << endl;
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

// 处理注册的响应逻辑
void DoRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()){ // 注册失败
        cerr << "name is already exist, register error!" << endl;
    } else {// 注册成功
        cout << "name register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}

// 显示当前登录成功用户的基本信息
void ShowCurrentUserData(){
    cout << "=============================login user=============================" << endl;
    cout << "current login user => id:" << g_currentUser.GetId() << ", name" << g_currentUser.GetName() << endl;
    cout << "-----------------------------friend list-----------------------------" << endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user : g_currentUserFriendList){
            cout << user.GetId() << " " << user.GetName() << " " << user.GetStat() << endl;
        }
    }

    cout << "-----------------------------group list-----------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.GetID() << " " << group.GetName() << " " << group.GetDesc() << endl;
            for (GroupUser &user : group.GetUsers())
            {
                cout << user.GetId() << " " << user.GetName() << " " << user.GetStat()
                     << " " << user.GetRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// 聊天主菜单页面
void MainMenu(int clientfd){
    Help();

    char buffer[1024] = {0};
    while(isMainMenuRunning){
        cin.getline(buffer, 1024);
        string commandbuf(buffer);// 直接使用字符数组构造string字符串
        string command;// 存储命令
        int idx = commandbuf.find(":");
        if(-1 == idx){
            command = commandbuf;
        }else{
            command = commandbuf.substr(0, idx);
        }
        auto iter = commandHandlerMap.find(command);// 寻找回调函数
        if(iter == commandHandlerMap.end()){
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调,MainMenu对修改封闭，添加新功能不需要修改该函数
        iter->second(clientfd, commandbuf.substr(idx+1, commandbuf.size() - idx));// 回调函数
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string GetCurrentTime(){
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year+1900, 
            (int)ptm->tm_mon+1,
            (int)ptm->tm_mday,
            (int)ptm->tm_hour,
            (int)ptm->tm_min,
            (int)ptm->tm_sec);
    return std::string(date);
}

// "help" command handler
void Help(int, string){
    cout << "show command list >>>>>" << endl;
    for(auto &p : commandMap){
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}
// "chat" command handler
void Chat(int clientfd, string str){
    int idx = str.find(":");// friendid:message
    if(-1 == idx){
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1, str.length()-idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.GetId();
    js["name"] = g_currentUser.GetName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = GetCurrentTime();

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send chat msg error ->" << buffer << endl;// 打印消息，方便排查错误
    }
}

// "addfriend" command handler
void AddFriend(int clientfd, string str){
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.GetId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
}

// "creategroup" command handler
void CreateGroup(int clientfd, string str){
    int idx = str.find(":");
    if(-1 == idx){
        cerr << "creategroup command invalid" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.GetId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send creategroup msg error=>" << buffer << endl;
    }
}

// "addgroup" command handler
void AddGroup(int clientfd, string str){
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.GetId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send addgroup msg error-> " << buffer << endl;
    }
}

// "groupchat" command handler
void GroupChat(int clientfd, string str){
    int idx = str.find(":");
    if(-1 == idx){
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.GetId();
    js["name"] = g_currentUser.GetName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = GetCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send groupchat msg error ->" << buffer << endl;
    }
}

// "loginout" command handler
void LoginOut(int clientfd, string str){
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.GetId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len){
        cerr << "send loginout msg error ->" << buffer << endl;
    }else{
        isMainMenuRunning = false;
    }
}
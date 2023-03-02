#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <cstdio>
#include <cstring>
#include <list>
#include <time.h>
#pragma comment (lib,"ws2_32.lib")
#define STR_MAX_LEN 200
#define NAME_LEN        15
#define SEND_LEN        STR_MAX_LEN + NAME_LEN + 2

using namespace std;

const int   MAX_ONLINE   =  20;
const char  Server_IP[]  =  "127.0.0.1",//改成你服务器的内网地址!!!!!
                Warn_Buf[]   =  "It's over Max connect!";
list < SOCKET > online;
bool            online_using =  0;

void Wrong_exit(const char* Error_place, int _Code) {
    printf("%s wrong!(at: %d)\n", Error_place, _Code);
    system("pause");
    exit(_Code);
}

void show_time() {
    static char time_l[20], time_n[20];
    static time_t timep;
   {
       time(&timep);
       strftime(time_n, sizeof(time_n), "%Y-%m-%d %H:%M", localtime(&timep));
       if (strcmp(time_l, time_n)) {
           strcpy(time_l, time_n);
           printf("//             %s\n", time_n);
      }
   }
}

void broadcast(const char* name, const char* send_buf) {
    static char ls_send[SEND_LEN];

    strcpy(ls_send, name);
    strcat(ls_send, ": ");
    strcat(ls_send, send_buf);

    while(online_using);
    online_using = 1;

    for(auto i = online.begin();i != online.end();i++)
        send(*i, ls_send, SEND_LEN, 0);

    online_using = 0;
}

void flash_online_num() {
    static char title[100];
    if(online.empty()) strcpy(title, "title=Connect-SAXI-Server");
    else sprintf(title, "title=Connect-SAXI-Server online_now:: %d", online.size());
    system(title);
}

DWORD WINAPI child_thread(LPVOID V_sock) {
    char name       [NAME_LEN];
    char recvbuf    [STR_MAX_LEN];

    SOCKET hsock = (SOCKET) V_sock;
    recv(hsock, name, NAME_LEN, 0);

    show_time();
    printf("%s login Successful!\n", name);
    flash_online_num();
    while(true) {
        recv(hsock, recvbuf, STR_MAX_LEN, 0);
        if(!strcmp(recvbuf, "ord::EXIT")) break;
        broadcast(name, recvbuf);
    }
    online_using = 1;
    online.remove(hsock);
    online_using = 0;

    show_time();
    printf("%s exited!\n", name);
    flash_online_num();
    return 0;
}

void NewHandle(SOCKET &SockFrom) {
    HANDLE  ls_handle;
    DWORD    ls_handle_id;
    ls_handle = (HANDLE)::CreateThread(NULL, 0,
            child_thread, (LPVOID)SockFrom, 0, &ls_handle_id);
}

int main() {
    system("title=Connect-SAXI-Server");
    puts("Connect-SAXI-Server");
    puts("                                        powerd by saxiy");

    WSADATA wsd;
   WSAStartup(MAKEWORD(2, 2), &wsd);
   SOCKET SockServer;
   sockaddr_in ServerAddr, FromAddr;

   ServerAddr.sin_family                =   AF_INET;
   ServerAddr.sin_port                  =   htons(5327);
   ServerAddr.sin_addr.S_un.S_addr  =   inet_addr(Server_IP);

   SockServer = socket(AF_INET, SOCK_STREAM, 0);

   if(bind(SockServer, (sockaddr*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
        Wrong_exit("bind", 1);

   if(listen(SockServer, MAX_ONLINE) == SOCKET_ERROR)
    Wrong_exit("listen", 2);

   int Socklen = sizeof(sockaddr);
   while(true) {
        SOCKET SockFrom;
    SockFrom = accept(SockServer, (sockaddr*)&FromAddr, &Socklen);
    if(SockFrom != INVALID_SOCKET) {
            if(online.size() < MAX_ONLINE) {
                while(online_using);
                online_using = 1;
                online.push_back(SockFrom);
                online_using = 0;
                NewHandle(SockFrom);
            } else {
                send(SockFrom, Warn_Buf, int(sizeof(Warn_Buf)/sizeof(char)), 0);
                closesocket(SockFrom);
            }
        }
    }
   WSACleanup();
    return 0;
}


#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <cstring>
#include <time.h>
#pragma comment (lib,"ws2_32.lib")
#define W_SIZE          650
#define STR_MAX_LEN 200
#define MEM_STR_LEN 10000
#define IN_TEXT     101
#define OUT_TEXT        201
#define B_DOWN          301

using namespace std;

const char  o_IP[]      =   "127.0.0.1",//改成服务器的映射地址!!!!!
            I_name[]        =   "unnamed";
char    sendbuf[STR_MAX_LEN],
        recvbuf[STR_MAX_LEN],
        memobuf[MEM_STR_LEN];
bool outlock = 0;
SOCKET connection;

void hwndOutput_add_buf(HWND, const char*, bool n = 1);

void show_time(HWND hwndout) {
    static char time_l[20], time_n[20];
    static time_t timep;
    {
        time(&timep);
        strftime(time_n, sizeof(time_n), "%Y-%m-%d %H:%M", localtime(&timep));
        if (strcmp(time_l, time_n)) {
            strcpy(time_l, time_n);
            strcat(memobuf, "//             ");
            strcat(memobuf, time_n);
            strcat(memobuf, "\r\n");
        }
    }
}

SOCKET gotsock(HWND hwndmind) {
    WSADATA wsd;
    WSAStartup(MAKEWORD(2, 2), &wsd);
    SOCKET SockUser;
    sockaddr_in ServerAddr;

    ServerAddr.sin_family               =   AF_INET;
    ServerAddr.sin_port                 =   htons(5327);
    ServerAddr.sin_addr.S_un.S_addr =   inet_addr(o_IP);

    SockUser = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(SockUser, (sockaddr*)&ServerAddr, sizeof(sockaddr)) != SOCKET_ERROR) {
        strcat(memobuf, "//========服务器连接成功========//\r\n");
        hwndOutput_add_buf(hwndmind, "", 0);
        return SockUser;
    } else {
        MessageBox(NULL, "服务器不在线!","Error!", MB_ICONEXCLAMATION | MB_OK);
        exit(0);
    }
}

void w_send(HWND hwndin, HWND hwndlog) {
    GetWindowText(hwndin, sendbuf, STR_MAX_LEN);
    SetWindowText(hwndin, "");

    if(!strcmp(sendbuf, "ord::cls")) {
        strcpy(memobuf, "//==========已清理内存==========//");
        hwndOutput_add_buf(hwndlog, "");
        return;
    }

    //add your new orders here

    send(connection, sendbuf, STR_MAX_LEN, 0);
    if(!strcmp(sendbuf, "ord::EXIT")) exit(0);
}

DWORD WINAPI w_recv(LPVOID hwndout) {
    HWND hwndOut = (HWND) hwndout;
    int noxtime = 0;
    while(noxtime < 3) {
        recv(connection, recvbuf, STR_MAX_LEN, 0);
        if(*recvbuf) {
            noxtime = 0;
            hwndOutput_add_buf(hwndOut, recvbuf);
        } else noxtime++;
        *recvbuf = '\0';
    }
    hwndOutput_add_buf(hwndOut, "//=!服务器已下线,3秒后退出程序!=//");
    Sleep(3000); exit(0);
}

void hwndOutput_add_buf(HWND hwnd, const char* buf, bool n) {
    static int len;

    while(outlock);
    outlock = 1;

    len = strlen(memobuf);
    if(len > MEM_STR_LEN - 500) strcpy(memobuf, "//=========自动清理内存=========//");
    show_time(hwnd);
    if(len > MEM_STR_LEN - 1000) strcpy(memobuf, "//=!即将清理历史;请注意保存数据!=//");

    strcat(memobuf, buf);
    if(n) strcat(memobuf, "\r\n");
    SetWindowText(hwnd, memobuf);
    SendMessage(hwnd, EM_LINESCROLL, 0, Edit_GetLineCount(hwnd));

    outlock = 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    static HDC              hdc;
    static PAINTSTRUCT  ps;
    static HANDLE           wc_listen;
    static DWORD            wc_listenID = 0;
    static HWND             B_send;
    static HWND             hwndOutput,
                                hwndInput;
    static int              wmID,
                            wmEvent;

    switch(Message) {
        case WM_CREATE: {
            unsigned int std_l = W_SIZE * 0.618 - 83;

            hwndOutput = CreateWindow( TEXT("edit"), NULL,
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
                                       ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                                       0, 0, W_SIZE - 20, std_l,
                                       hwnd, (HMENU)OUT_TEXT, ((LPCREATESTRUCT) lParam) -> hInstance, NULL );

            hwndInput = CreateWindow( TEXT("edit"), NULL,
                                      WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_BORDER |
                                      ES_LEFT | ES_AUTOHSCROLL,
                                      0, std_l, W_SIZE - 70, 40,
                                      hwnd, (HMENU)IN_TEXT, ((LPCREATESTRUCT) lParam) -> hInstance, NULL );

            HWND B_send=CreateWindow( TEXT("BUTTON"), TEXT("Send"),
                                      WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                      W_SIZE - 70, std_l, 50, 40,
                                      hwnd, (HMENU)B_DOWN, (HINSTANCE)GetWindowLong(hwnd, -6), NULL);

            connection = gotsock(hwndOutput);
            send(connection, I_name, int(sizeof(I_name) / sizeof(char)), 0);
            Edit_LimitText(hwndInput, STR_MAX_LEN);
            wc_listen = (HANDLE)::CreateThread(NULL, 0, w_recv, (LPVOID)hwndOutput, 0, &wc_listenID);

            break;
        }
        case WM_PAINT: {
            hdc = BeginPaint(hwnd, &ps);

            //add your paint code here

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_COMMAND: {
            wmID        =   LOWORD(wParam);
            wmEvent =   HIWORD(wParam);
            switch(wmID) {
                case B_DOWN: {
                    w_send(hwndInput, hwndOutput);
                    break;
                }
                //other commands
            }
            break;
        }
        case WM_DESTROY: {
            send(connection, "ord::EXIT", STR_MAX_LEN, 0);
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, Message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    memset(&wc, 0, sizeof(wc));

    wc.cbSize           =   sizeof(WNDCLASSEX);
    wc.lpfnWndProc      =   WndProc;
    wc.hInstance        =   hInstance;
    wc.hCursor          =   LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    =   (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName    =   "Win_connect";
    wc.hIcon                =   LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm          =   LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!","Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "Win_connect", "connect", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,      //     x     //
                          CW_USEDEFAULT,      //     y     //
                          W_SIZE,             //   width   //
                          W_SIZE * 0.618,     //   height  //
                          NULL, NULL, hInstance, NULL);

    if(hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    //Message Rounds
    while(GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}


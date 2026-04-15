// Client.cpp
#include "NetLib.h"
#include <windows.h>
#include <stdio.h>


#define IDC_IP_EDIT     101
#define IDC_PORT_EDIT   102
#define IDC_NAME_EDIT   103
#define IDC_CONNECT_BTN 104
#define IDC_CHAT_VIEW   105
#define IDC_MSG_EDIT    106
#define IDC_SEND_BTN    107

HWND g_hIPEdit, g_hPortEdit, g_hNameEdit, g_hIDView, g_hChatView, g_hMsgEdit, g_hWnd;
bool g_connected = false;

void AddChatMessage(const char* msg) {
    SendMessageA(g_hChatView, LB_ADDSTRING, 0, (LPARAM)msg);
    SendMessage(g_hChatView, LB_SETTOPINDEX, SendMessage(g_hChatView, LB_GETCOUNT, 0, 0) - 1, 0);
}

// 패킷 수신 콜백
void ProcessPacket(char* buf) {
    PacketHeader* h = (PacketHeader*)buf;

    // 명세 2.2: 서버가 ID 전달 -> 클라이언트가 이름 전송
    if (h->type == PKT_CONNECT_ACK) {
        PktConnectAck* ack = (PktConnectAck*)buf;

        // 화면에 ID 표시 (0x... 형태)
        char idStr[32];
        sprintf_s(idStr, "0x%08X", ack->id);
        SetWindowTextA(g_hIDView, idStr);
        AddChatMessage("[시스템] 서버에 접속되었습니다.");

        // 이름 전송 (PKT_CONNECT)
        char name[20];
        GetWindowTextA(g_hNameEdit, name, 20);

        PktConnect req;
        req.header = { PACKET_MAGIC, sizeof(PktConnect), PKT_CONNECT };
        strncpy_s(req.name, name, 19);
        Net_Send(&req);
    }
    else if (h->type == PKT_CHAT_BROADCAST) {
        PktChatBroadcast* bc = (PktChatBroadcast*)buf;
        char line[300];
        sprintf_s(line, "[%s] %s", bc->name, bc->msg);
        AddChatMessage(line);
    }
}

DWORD WINAPI RecvThread(LPVOID) {
    while (g_connected) {
        if (Net_Recv() < 0) {
            AddChatMessage("[시스템] 서버와 연결이 종료되었습니다.");
            g_connected = false;
            break;
        }
    }
    return 0;
}

void OnConnect() {
    if (g_connected) return;

    char ip[64], portStr[16], name[20];
    GetWindowTextA(g_hIPEdit, ip, 64);
    GetWindowTextA(g_hPortEdit, portStr, 16);
    GetWindowTextA(g_hNameEdit, name, 20);

    if (strlen(name) == 0) { MessageBoxA(g_hWnd, "이름을 입력하세요.", "알림", MB_OK); return; }

    int port = strlen(portStr) > 0 ? atoi(portStr) : SERVERPORT;
    if (Net_Connect(strlen(ip) > 0 ? ip : "127.0.0.1", port) < 0) {
        MessageBoxA(g_hWnd, "연결 실패", "에러", MB_OK); return;
    }

    g_connected = true;
    CreateThread(NULL, 0, RecvThread, NULL, 0, NULL);
    // 참고: connect() 성공 직후 서버에서 PKT_CONNECT_ACK가 날아옵니다.
}

void OnSend() {
    if (!g_connected) return;
    char msg[256];
    GetWindowTextA(g_hMsgEdit, msg, 256);
    if (strlen(msg) == 0) return;

    PktChat pkt;
    pkt.header = { PACKET_MAGIC, sizeof(PktChat), PKT_CHAT };
    strncpy_s(pkt.msg, msg, 255);
    Net_Send(&pkt);

    SetWindowTextA(g_hMsgEdit, ""); // 입력창 비우기
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CREATE) {
        // UI 예시 이미지와 유사한 배치
        CreateWindowA("STATIC", "IP / Port", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 10, 15, 60, 25, hWnd, 0, 0, 0);
        g_hIPEdit = CreateWindowA("EDIT", "127.0.0.1", WS_CHILD | WS_VISIBLE | WS_BORDER, 80, 15, 120, 25, hWnd, (HMENU)IDC_IP_EDIT, 0, 0);
        g_hPortEdit = CreateWindowA("EDIT", "9000", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, 15, 60, 25, hWnd, (HMENU)IDC_PORT_EDIT, 0, 0);
        CreateWindowA("BUTTON", "connect", WS_CHILD | WS_VISIBLE, 280, 15, 80, 25, hWnd, (HMENU)IDC_CONNECT_BTN, 0, 0);

        CreateWindowA("STATIC", "name", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 10, 50, 60, 25, hWnd, 0, 0, 0);
        g_hNameEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 80, 50, 120, 25, hWnd, (HMENU)IDC_NAME_EDIT, 0, 0);

        CreateWindowA("STATIC", "ID", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 85, 60, 30, hWnd, 0, 0, 0);
        g_hIDView = CreateWindowA("STATIC", "대기중...", WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTERIMAGE, 80, 85, 190, 30, hWnd, 0, 0, 0);

        CreateWindowA("STATIC", "chat", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 10, 125, 60, 25, hWnd, 0, 0, 0);
        g_hMsgEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 80, 125, 270, 25, hWnd, (HMENU)IDC_MSG_EDIT, 0, 0);
        CreateWindowA("BUTTON", "send", WS_CHILD | WS_VISIBLE, 360, 125, 60, 25, hWnd, (HMENU)IDC_SEND_BTN, 0, 0);

        g_hChatView = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, 10, 160, 410, 180, hWnd, (HMENU)IDC_CHAT_VIEW, 0, 0);
        CreateWindowA("BUTTON", "종료", WS_CHILD | WS_VISIBLE, 360, 350, 60, 25, hWnd, (HMENU)SC_CLOSE, 0, 0);

        g_hWnd = hWnd;
    }
    else if (m == WM_COMMAND) {
        if (LOWORD(w) == IDC_CONNECT_BTN) OnConnect();
        else if (LOWORD(w) == IDC_SEND_BTN) OnSend();
        else if (LOWORD(w) == SC_CLOSE) SendMessage(hWnd, WM_CLOSE, 0, 0);
    }
    else if (m == WM_DESTROY) {
        g_connected = false;
        Net_Close();
        PostQuitMessage(0);
    }
    else return DefWindowProcA(hWnd, m, w, l);
    return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int n) {
    Net_Init();
    WNDCLASSA wc = { 0 }; wc.lpfnWndProc = WndProc; wc.hInstance = h;
    wc.lpszClassName = "ChatApp"; wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassA(&wc);

    HWND hWnd = CreateWindowA("ChatApp", "Network Chat Client", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 430, 0, 0, h, 0);
    ShowWindow(hWnd, n);

    MSG msg; while (GetMessage(&msg, 0, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
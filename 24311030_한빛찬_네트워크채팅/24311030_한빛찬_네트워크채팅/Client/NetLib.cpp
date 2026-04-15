// NetLib.cpp
#include "NetLib.h"
#include <ws2tcpip.h>
#include <string.h>

static SOCKET  g_sock = INVALID_SOCKET;
static char    g_buf[4096];
static int     g_remain = 0;

int Net_Init() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa);
}

int Net_Connect(const char* ip, int port) {
    g_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sock == INVALID_SOCKET) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    if (connect(g_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        return -1;
    }
    return 0;
}

int Net_Send(void* pkt) {
    if (g_sock == INVALID_SOCKET) return -1;
    PacketHeader* h = (PacketHeader*)pkt;
    int total = 0;
    while (total < h->size) {
        int ret = send(g_sock, (char*)pkt + total, h->size - total, 0);
        if (ret == SOCKET_ERROR) return -1;
        total += ret;
    }
    return 0;
}

int Net_Recv() {
    if (g_sock == INVALID_SOCKET) return -1;
    int sz = recv(g_sock, g_buf + g_remain, sizeof(g_buf) - g_remain, 0);
    if (sz <= 0) return -1;
    g_remain += sz;

    while (g_remain >= (int)sizeof(PacketHeader)) {
        PacketHeader* h = (PacketHeader*)g_buf;
        if (h->magic != PACKET_MAGIC) return -1; // 연결 종료 처리
        if (g_remain < h->size) break;

        ProcessPacket(g_buf);

        int processed = h->size;
        memmove(g_buf, g_buf + processed, g_remain - processed);
        g_remain -= processed;
    }
    return 0;
}

void Net_Close() {
    if (g_sock != INVALID_SOCKET) {
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    WSACleanup();
}
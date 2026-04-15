// Server.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include "NetPacket.h"

#pragma comment(lib, "ws2_32.lib")

struct Client {
    int id;
    SOCKET sock;
    char name[20];
    char buf[4096]; // 클라이언트별 독립 수신 버퍼
    int remain = 0;
};

std::vector<Client> g_clients;

void Broadcast(void* pkt) {
    PacketHeader* h = (PacketHeader*)pkt;
    for (auto& c : g_clients) {
        send(c.sock, (char*)pkt, h->size, 0);
    }
}

void ProcessServerPacket(SOCKET s, char* buf) {
    PacketHeader* h = (PacketHeader*)buf;

    // 명세 2.2: 클라이언트가 보낸 이름 수신 및 저장
    if (h->type == PKT_CONNECT) {
        PktConnect* pkt = (PktConnect*)buf;
        for (auto& c : g_clients) {
            if (c.sock == s) {
                strncpy_s(c.name, pkt->name, 19);
                printf("[알림] 사용자 이름 등록 완료: %s (ID: 0x%08X)\n", c.name, c.id);
                break;
            }
        }
    }
    // 채팅 수신 및 Broadcast 재구성
    else if (h->type == PKT_CHAT) {
        PktChat* pkt = (PktChat*)buf;
        PktChatBroadcast bc;
        bc.header = { PACKET_MAGIC, sizeof(bc), PKT_CHAT_BROADCAST };
        strncpy_s(bc.msg, pkt->msg, 255);

        // 보낸 사람 찾기
        strcpy_s(bc.name, "unknown");
        for (auto& c : g_clients) {
            if (c.sock == s) { strncpy_s(bc.name, c.name, 19); break; }
        }

        printf("[채팅] %s: %s\n", bc.name, bc.msg);
        Broadcast(&bc);
    }
}

int main() {
    system("chcp 65001");

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = { AF_INET, htons(SERVERPORT) };
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, SOMAXCONN);

    printf("========= 채팅 서버 가동 (Port: %d) =========\n", SERVERPORT);

    while (1) {
        fd_set reads; FD_ZERO(&reads); FD_SET(listenSock, &reads);
        SOCKET maxS = listenSock;

        for (auto& c : g_clients) {
            FD_SET(c.sock, &reads);
            if (c.sock > maxS) maxS = c.sock;
        }

        if (select((int)maxS + 1, &reads, 0, 0, 0) <= 0) break;

        // 명세 2.2: 클라이언트 접속 -> 서버가 ID 생성 및 즉시 전달
        if (FD_ISSET(listenSock, &reads)) {
            SOCKET clientSock = accept(listenSock, 0, 0);

            Client c = { (int)clientSock, clientSock, "unknown", {0}, 0 };
            g_clients.push_back(c);

            printf("[알림] 클라이언트 접속 (Socket: %llu)\n", clientSock);

            // 접속한 클라이언트에게 ID 전달 (PKT_CONNECT_ACK)
            PktConnectAck ack = { {PACKET_MAGIC, sizeof(ack), PKT_CONNECT_ACK}, c.id };
            send(clientSock, (char*)&ack, sizeof(ack), 0);
        }

        // 데이터 수신 (TCP 안전 분리)
        for (int i = (int)g_clients.size() - 1; i >= 0; i--) {
            Client& c = g_clients[i];
            if (FD_ISSET(c.sock, &reads)) {
                int r = recv(c.sock, c.buf + c.remain, sizeof(c.buf) - c.remain, 0);
                if (r <= 0) {
                    printf("[알림] 클라이언트 종료: %s\n", c.name);
                    closesocket(c.sock);
                    g_clients.erase(g_clients.begin() + i);
                    continue;
                }
                c.remain += r;

                while (c.remain >= (int)sizeof(PacketHeader)) {
                    PacketHeader* h = (PacketHeader*)c.buf;
                    if (h->magic != PACKET_MAGIC) {
                        closesocket(c.sock); // 매직넘버 오류 시 끊기
                        break;
                    }
                    if (c.remain < h->size) break;

                    ProcessServerPacket(c.sock, c.buf);

                    memmove(c.buf, c.buf + h->size, c.remain - h->size);
                    c.remain -= h->size;
                }
            }
        }
    }
    return 0;
}
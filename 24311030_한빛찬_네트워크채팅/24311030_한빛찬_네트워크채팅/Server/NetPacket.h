// NetPacket.h
#pragma once
#include <stdint.h>

#define PACKET_MAGIC 0x50434B54  // 'PCKT'
#define SERVERPORT   9000

#pragma pack(push, 1)

// ─── 패킷 헤더 ───
struct PacketHeader
{
    uint32_t magic;     // 패킷 구분자
    uint16_t size;      // 헤더 포함 패킷 전체 길이
    uint32_t type;      // 패킷 type
};

// ─── 패킷 타입 ───
enum PacketType
{
    PKT_CONNECT = 1,
    PKT_CONNECT_ACK = 2,
    PKT_CHAT = 0x1001,
    PKT_CHAT_BROADCAST = 0x2001,
};

// ─── 패킷 정의 ───

// 접속 요청 (클라이언트 -> 서버로 이름 전송)
struct PktConnect
{
    PacketHeader header;
    char name[20]; // 최대 19자 + null
};

// 접속 응답 (서버 -> 클라이언트로 ID 전달)
struct PktConnectAck
{
    PacketHeader header;
    int id;
};

// 채팅 메시지 (클라이언트 -> 서버)
struct PktChat
{
    PacketHeader header;
    char msg[256];
};

// 브로드캐스트 메시지 (서버 -> 클라이언트)
struct PktChatBroadcast
{
    PacketHeader header;
    char name[20];
    char msg[256];
};

#pragma pack(pop)
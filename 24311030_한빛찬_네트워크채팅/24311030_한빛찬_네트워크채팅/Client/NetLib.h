// NetLib.h
#pragma once
#include <winsock2.h>
#include "NetPacket.h"

#pragma comment(lib, "ws2_32.lib")

int  Net_Init();
int  Net_Connect(const char* ip, int port);
int  Net_Send(void* pkt);
int  Net_Recv();
void Net_Close();

// 수신 패킷 처리 콜백 (Client.cpp에서 구현)
void ProcessPacket(char* buf);
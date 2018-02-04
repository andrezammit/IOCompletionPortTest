// IOCompletionPortTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <ws2tcpip.h>

#include <thread>
#include <vector>
#include <list>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

class CClientSocket
{
public:
	CClientSocket()
	{
		m_socket = INVALID_SOCKET;
		ZeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));

		m_buffer.resize(1024);
	}
	
	bool operator==(const CClientSocket& rhs)
	{
		return this == &rhs; 
	}

	SOCKET m_socket;
	WSAOVERLAPPED m_overlapped;

	vector<BYTE> m_buffer;
};

HANDLE m_completionPort = NULL;
vector<CClientSocket> m_clientSockets;

void CloseClientSocket(CClientSocket& clientSocket)
{
	auto it = std::find(m_clientSockets.begin(), m_clientSockets.end(), clientSocket);

	if (it != m_clientSockets.end())
	{
		m_clientSockets.erase(it);
	}

	closesocket(clientSocket.m_socket);
}

bool ReceiveFromSocket(CClientSocket& clientSocket)
{
	WSABUF dataBuff;

	dataBuff.len = clientSocket.m_buffer.size();
	dataBuff.buf = (char*)&clientSocket.m_buffer[0];

	DWORD flags = 0;

	int ret = WSARecv(clientSocket.m_socket, &dataBuff,
		1, NULL, &flags,
		&clientSocket.m_overlapped, NULL);

	if (ret != 0)
	{
		int error = WSAGetLastError();
		
		if (error != WSA_IO_PENDING)
		{
			CloseClientSocket(clientSocket);
			return false;
		}
	}

	return true;
}

bool SendOnSocket(CClientSocket& clientSocket, DWORD bytesToSend)
{
	WSABUF dataBuff;

	dataBuff.len = clientSocket.m_buffer.size();
	dataBuff.buf = (char*)&clientSocket.m_buffer[0];

	DWORD flags = 0;

	WSASend(clientSocket.m_socket, &dataBuff, 1, &bytesToSend, WSA_FLAG_OVERLAPPED, &clientSocket.m_overlapped, NULL);
}

void ProcessThreadFunc()
{
	while (true)
	{
		DWORD bytesWritten = 0;

		OVERLAPPED* pOverlapped = NULL;
		CClientSocket* pClientSocket = NULL;

		GetQueuedCompletionStatus(m_completionPort, &bytesWritten, (PULONG_PTR) &pClientSocket, &pOverlapped, INFINITE);
		
		if (pClientSocket == NULL)
		{
			continue;
		}

		CClientSocket& clientSocket = *pClientSocket;

		if (!ReceiveFromSocket(clientSocket))
		{
			continue;
		}

		//SendOnSocket(clientSocket, bytesWritten);
		send(pClientSocket->m_socket, (char*)&pClientSocket->m_buffer[0], bytesWritten, 0);
	}
}

int main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	thread processThread1(ProcessThreadFunc);
	thread processThread2(ProcessThreadFunc);

	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);

	sockaddr_in service;
	
	service.sin_family = AF_INET;
	service.sin_port = htons(880);
	
	InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);

	::bind(listenSocket, (SOCKADDR*) &service, sizeof(service));
	listen(listenSocket, SOMAXCONN);

	while (true)
	{
		sockaddr_in clientAddress = { 0 };
		int clientAddrLength = sizeof(clientAddress);

		SOCKET socket = accept(listenSocket, (sockaddr*)&clientAddress, &clientAddrLength);

		m_clientSockets.push_back(CClientSocket());

		CClientSocket& clientSocket = m_clientSockets.back();
		clientSocket.m_socket = socket;

		CreateIoCompletionPort((HANDLE) socket, m_completionPort, (DWORD_PTR) &clientSocket, 0);

		ReceiveFromSocket(clientSocket);
	}

	processThread1.join();
	processThread2.join();

	closesocket(listenSocket);

	WSACleanup();
    return 0;
}


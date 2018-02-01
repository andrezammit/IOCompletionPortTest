// IOCompletionPortTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <ws2tcpip.h>

#include <thread>
#include <vector>
#include <list>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

HANDLE m_completionPort = NULL;

class CWorkItem
{
public:
	CWorkItem()
	{
		m_id = ++m_nextId;

		m_counter = 0;
	}

	DWORD m_id;
	DWORD m_counter;

private:
	static DWORD m_nextId;
};

DWORD CWorkItem::m_nextId = 0;

class CClientSocket
{
public:
	CClientSocket()
	{
		m_socket = INVALID_SOCKET;
	}

	SOCKET m_socket;
};

void ProcessThreadFunc()
{
	while (true)
	{
		DWORD bytesWritten = 0;

		SOCKET* pSocket = NULL;
		OVERLAPPED* pOverlapped = NULL;
		
		GetQueuedCompletionStatus(m_completionPort, &bytesWritten, (PULONG_PTR) &pSocket, &pOverlapped, INFINITE);
		
		if (pSocket == NULL)
		{
			continue;
		}

		char buffer[16];
		recv(*pSocket, buffer, sizeof(buffer), MSG_WAITALL);

		send(*pSocket, buffer, sizeof(buffer), 0);
	}
}

int main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	thread processThread1(ProcessThreadFunc);
	thread processThread2(ProcessThreadFunc);

	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	sockaddr_in service;
	service.sin_family = AF_INET;
	InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
	service.sin_port = htons(880);

	int ret = ::bind(listenSocket, (SOCKADDR*) &service, sizeof(service));

	ret = listen(listenSocket, SOMAXCONN);

	fd_set readSet;
	FD_ZERO(&readSet);

	FD_SET(listenSocket, &readSet);

	list<SOCKET> clientSockets;

	while (true)
	{
		select(0, &readSet, NULL, NULL, 0);

		CClientSocket* pCurrentSocket = NULL;

		if (FD_ISSET(listenSocket, &readSet))
		{
			sockaddr_in clientAddress = { 0 };
			int clientAddrLength = sizeof(clientAddress);

			SOCKET socket = accept(listenSocket, (sockaddr*) &clientAddress, &clientAddrLength);
			clientSockets.push_back(socket);

			FD_SET(socket, &readSet);

			PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)&socket, NULL);
		}
		else
		{
			for (auto socket : clientSockets)
			{
				if (FD_ISSET(socket, &readSet))
				{
					PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)&socket, NULL);
				}
			}
		}
	}

	processThread1.join();
	processThread2.join();

	closesocket(listenSocket);

	WSACleanup();
    return 0;
}


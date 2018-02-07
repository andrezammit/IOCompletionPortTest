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
	static const int modeRead = 1;
	static const int modeWrite = 2;

	CClientSocket()
	{
		m_transferMode = modeRead;

		m_bytesDone = 0;
		m_packetLen = 0;

		m_socket = INVALID_SOCKET;
		ZeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));

		m_buffer.resize(1024);
	}
	
	bool operator==(const CClientSocket& rhs)
	{
		return this == &rhs; 
	}

	void SetPacketLength(DWORD packetLen)
	{
		m_packetLen = packetLen;
		m_buffer.resize(packetLen);
	}

	bool IsPacketTransferred()
	{
		return m_bytesDone == m_packetLen;
	}

	void ResetTransferredBytes()
	{
		m_bytesDone = 0;
	}

	void AddTransferredBytes(DWORD bytesTransferred)
	{
		m_bytesDone += bytesTransferred;
	}

	bool IsReadMode()
	{
		return m_transferMode == modeRead;
	}

	bool IsWriteMode()
	{
		return m_transferMode == modeWrite;
	}

	void SetReadMode()
	{
		m_transferMode = modeRead;
	}

	void SetWriteMode()
	{
		m_transferMode = modeWrite;
	}

	DWORD m_transferMode;

	DWORD m_bytesDone;
	DWORD m_packetLen;

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
	DWORD bufferPos = clientSocket.m_bytesDone;

	WSABUF dataBuff;

	dataBuff.len = clientSocket.m_buffer.size() - bufferPos;
	dataBuff.buf = (char*)&clientSocket.m_buffer[bufferPos];

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

bool SendOnSocket(CClientSocket& clientSocket)
{
	DWORD bufferPos = clientSocket.m_bytesDone;

	WSABUF dataBuff;

	dataBuff.len = clientSocket.m_buffer.size() - bufferPos;
	dataBuff.buf = (char*)&clientSocket.m_buffer[bufferPos];

	DWORD flags = 0;

	WSASend(clientSocket.m_socket, &dataBuff, 1, NULL, flags, &clientSocket.m_overlapped, NULL);

	return true;
}

void ProcessThreadFunc()
{
	while (true)
	{
		DWORD bytesTransferred = 0;

		OVERLAPPED* pOverlapped = NULL;
		CClientSocket* pClientSocket = NULL;

		GetQueuedCompletionStatus(m_completionPort, &bytesTransferred, (PULONG_PTR) &pClientSocket, &pOverlapped, INFINITE);
		
		if (pClientSocket == NULL)
		{
			continue;
		}

		CClientSocket& clientSocket = *pClientSocket;
		clientSocket.AddTransferredBytes(bytesTransferred);

		if (clientSocket.IsPacketTransferred())
		{
			clientSocket.ResetTransferredBytes();

			if (clientSocket.IsReadMode())
			{
				clientSocket.SetWriteMode();
			}
			else
			{
				clientSocket.SetReadMode();
			}
		}

		if (clientSocket.IsReadMode() && !ReceiveFromSocket(clientSocket))
		{
			continue;
		}

		if (clientSocket.IsWriteMode() && !SendOnSocket(clientSocket))
		{
			continue;
		}
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
		
		clientSocket.SetPacketLength(5);

		CreateIoCompletionPort((HANDLE) socket, m_completionPort, (DWORD_PTR) &clientSocket, 0);

		ReceiveFromSocket(clientSocket);
	}

	processThread1.join();
	processThread2.join();

	closesocket(listenSocket);

	WSACleanup();
    return 0;
}


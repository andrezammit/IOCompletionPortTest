#include "stdafx.h"
#include "MainApp.h"

HANDLE CMainApp::m_completionPort = NULL;
vector<CClientSocket> CMainApp::m_clientSockets;

CMainApp::CMainApp()
{
	m_listenSocket = INVALID_SOCKET;

	Init();
}

CMainApp::~CMainApp()
{
	Uninit();
}

void CMainApp::Run()
{
	if (!CreateIOCompletionPort())
	{
		return;
	}

	if (!StartServer())
	{
		return;
	}

	StartThreads();

	while (true)
	{
		sockaddr_in clientAddress = { 0 };
		int clientAddrLength = sizeof(clientAddress);

		SOCKET socket = accept(m_listenSocket, (sockaddr*)&clientAddress, &clientAddrLength);

		m_clientSockets.push_back(CClientSocket());

		CClientSocket& clientSocket = m_clientSockets.back();
		clientSocket.m_socket = socket;

		clientSocket.SetPacketLength(5);

		UpdateIOCompletionPort(clientSocket);

		clientSocket.Receive();
	}
}

bool CMainApp::Init()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	return true;
}

void CMainApp::Uninit()
{
	closesocket(m_listenSocket);

	WSACleanup();
}

bool CMainApp::StartServer()
{
	m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_listenSocket == INVALID_SOCKET)
	{
		return false;
	}

	sockaddr_in service;

	service.sin_family = AF_INET;
	service.sin_port = htons(880);

	InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);

	::bind(m_listenSocket, (SOCKADDR*)&service, sizeof(service));
	listen(m_listenSocket, SOMAXCONN);

	return true;
}

bool CMainApp::CreateIOCompletionPort()
{
	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	if (m_completionPort == NULL)
	{
		return false;
	}

	return true;
}

bool CMainApp::UpdateIOCompletionPort(CClientSocket& clientSocket)
{
	HANDLE completionPort = CreateIoCompletionPort((HANDLE) clientSocket.m_socket, m_completionPort, (ULONG_PTR)&clientSocket, 2);

	if (m_completionPort != m_completionPort)
	{
		return false;
	}

	return true;
}

void CMainApp::StartThreads()
{
	int numOfThreads = 2;

	for (int cnt = 0; cnt < numOfThreads; cnt++)
	{
		unique_ptr<thread> pThread(new thread(ProcessThreadFunc));
		m_threads.emplace_back(move(pThread));
	}
}

void CMainApp::StopThreads()
{
	for (size_t cnt = 0; cnt < m_threads.size(); cnt++)
	{
		m_threads[cnt]->join();
	}
}

void CMainApp::ProcessThreadFunc()
{
	while (true)
	{
		DWORD bytesTransferred = 0;

		OVERLAPPED* pOverlapped = NULL;
		CClientSocket* pClientSocket = NULL;

		GetQueuedCompletionStatus(m_completionPort, &bytesTransferred, (PULONG_PTR)&pClientSocket, &pOverlapped, INFINITE);

		if (pClientSocket == NULL)
		{
			continue;
		}

		CClientSocket& clientSocket = *pClientSocket;

		if (bytesTransferred == 0)
		{
			CloseClientSocket(clientSocket);
			continue;
		}

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

		clientSocket.Receive();
		clientSocket.Send();
	}
}

void CMainApp::CloseClientSocket(CClientSocket& clientSocket)
{
	auto it = std::find(m_clientSockets.begin(), m_clientSockets.end(), clientSocket);

	if (it != m_clientSockets.end())
	{
		m_clientSockets.erase(it);
	}
}
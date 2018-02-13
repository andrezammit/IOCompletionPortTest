#include "stdafx.h"
#include "MainApp.h"

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

		ClientSocketShPtr pClientSocket(new CClientSocket);
		pClientSocket->m_socket = socket;
		pClientSocket->SetPacketLength(5);

		m_clientSockets[pClientSocket->m_socket] = pClientSocket;

		UpdateIOCompletionPort(*pClientSocket);

		pClientSocket->Receive();
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
	m_shutdown = true;

	closesocket(m_listenSocket);

	CloseIOCompletionPort();
	StopThreads();

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

void CMainApp::CloseIOCompletionPort()
{
	CloseHandle(m_completionPort);
	m_completionPort = NULL;
}

bool CMainApp::UpdateIOCompletionPort(CClientSocket& clientSocket)
{
	HANDLE completionPort = CreateIoCompletionPort((HANDLE) clientSocket.m_socket, m_completionPort, (ULONG_PTR)clientSocket.m_socket, 2);

	if (m_completionPort != completionPort)
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
		unique_ptr<thread> pThread(new thread(ProcessingThread, this));
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

void CMainApp::ProcessingThread(CMainApp* pMainApp)
{
	pMainApp->ProcessingThreadFunc();
}

void CMainApp::ProcessingThreadFunc()
{
	while (true)
	{
		DWORD bytesTransferred = 0;

		SOCKET socket = INVALID_SOCKET;
		OVERLAPPED* pOverlapped = nullptr;

		if (!GetQueuedCompletionStatus(m_completionPort, &bytesTransferred, (PULONG_PTR)&socket, &pOverlapped, INFINITE))
		{
			return;
		}

		if (m_shutdown)
		{
			return;
		}

		ClientSocketShPtr pClientSocket = m_clientSockets[socket];

		if (pClientSocket == nullptr)
		{
			continue;
		}

		if (bytesTransferred == 0)
		{
			CloseClientSocket(pClientSocket);
			continue;
		}

		pClientSocket->AddTransferredBytes(bytesTransferred);

		if (pClientSocket->IsPacketTransferred())
		{
			pClientSocket->ResetTransferredBytes();

			if (pClientSocket->IsReadMode())
			{
				pClientSocket->SetWriteMode();
			}
			else
			{
				pClientSocket->SetReadMode();
			}
		}

		pClientSocket->Receive();
		pClientSocket->Send();
	}
}

void CMainApp::CloseClientSocket(ClientSocketShPtr& pClientSocket)
{
	m_clientSockets[pClientSocket->m_socket] = nullptr;
}

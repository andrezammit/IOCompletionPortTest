#include "stdafx.h"
#include "MainApp.h"

CMainApp::CMainApp()
{
	m_listenSocket = INVALID_SOCKET;

	m_jobCompletionPort = NULL;
	m_socketCompletionPort = NULL;

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
		sockaddr_in6 clientAddress = { 0 };
		int clientAddrLength = sizeof(clientAddress);

		SOCKET socket = WSAAccept(m_listenSocket, (sockaddr*)&clientAddress, &clientAddrLength, NULL, NULL);

		ClientSocketShPtr pClientSocket(new CClientSocket(socket, (sockaddr&) clientAddress));

		_tprintf(_T("New socket %d from %s\n"), 
			pClientSocket->m_socket, 
			pClientSocket->GetClientAddrString().GetString());

		pClientSocket->m_socket = socket;
		pClientSocket->SetPacketLength(1);

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
	_tprintf(_T("Shutting down.\n"));

	m_shutdown = true;

	closesocket(m_listenSocket);

	CloseIOCompletionPort();
	StopThreads();

	WSACleanup();
}

bool CMainApp::StartServer()
{
	_tprintf(_T("Starting server on 127.0.0.1:880.\n"));

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
	_tprintf(_T("Creating IO completion port.\n"));

	m_jobCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	if (m_jobCompletionPort == NULL)
	{
		return false;
	}

	m_socketCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	if (m_socketCompletionPort == NULL)
	{
		return false;
	}

	return true;
}

void CMainApp::CloseIOCompletionPort()
{
	_tprintf(_T("Closing IO completion port.\n"));

	CloseHandle(m_jobCompletionPort);
	CloseHandle(m_socketCompletionPort);

	m_jobCompletionPort = NULL;
	m_socketCompletionPort = NULL;
}

bool CMainApp::UpdateIOCompletionPort(CClientSocket& clientSocket)
{
	HANDLE completionPort = CreateIoCompletionPort((HANDLE) clientSocket.m_socket, m_socketCompletionPort, (ULONG_PTR)clientSocket.m_socket, 2);

	if (m_socketCompletionPort != completionPort)
	{
		return false;
	}

	return true;
}

void CMainApp::StartThreads()
{
	_tprintf(_T("Starting threads.\n"));

	int numOfThreads = 2;

	for (int cnt = 0; cnt < numOfThreads; cnt++)
	{
		unique_ptr<thread> pThread(new thread(SocketProcessingThread, this));
		m_threads.emplace_back(move(pThread));
	}

	for (int cnt = 0; cnt < numOfThreads; cnt++)
	{
		unique_ptr<thread> pThread(new thread(JobProcessingThread, this));
		m_threads.emplace_back(move(pThread));
	}
}

void CMainApp::StopThreads()
{
	_tprintf(_T("Stopping threads.\n"));

	for (size_t cnt = 0; cnt < m_threads.size(); cnt++)
	{
		m_threads[cnt]->join();
	}
}

void CMainApp::SocketProcessingThread(CMainApp* pMainApp)
{
	pMainApp->SocketProcessingThreadFunc();
}

void CMainApp::SocketProcessingThreadFunc()
{
	while (true)
	{
		DWORD bytesTransferred = 0;

		SOCKET socket = INVALID_SOCKET;
		OVERLAPPED* pOverlapped = nullptr;

		if (!GetQueuedCompletionStatus(m_socketCompletionPort, &bytesTransferred, (PULONG_PTR)&socket, &pOverlapped, INFINITE))
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
			if (pClientSocket->IsReadMode())
			{
				pClientSocket->ResetTransferredBytes();

				QueuedJobShPtr pNewJob(new CJob(pClientSocket.get()));
				m_queuedJobs[pNewJob.get()] = pNewJob;

				PostQueuedCompletionStatus(m_jobCompletionPort, 0, (ULONG_PTR)pNewJob.get(), NULL);
			}
			else
			{
				pClientSocket->SetReadMode();
				pClientSocket->Receive();
			}
		}
		else
		{
			pClientSocket->Receive();
			pClientSocket->Send();
		}
	}
}

void CMainApp::CloseClientSocket(ClientSocketShPtr& pClientSocket)
{
	_tprintf(_T("Closing socket %d from %s.\n"), 
		pClientSocket->m_socket, 
		pClientSocket->GetClientAddrString().GetString());

	m_clientSockets[pClientSocket->m_socket] = nullptr;
}

void CMainApp::JobProcessingThread(CMainApp* pMainApp)
{
	pMainApp->JobProcessingThreadFunc();
}

void CMainApp::JobProcessingThreadFunc()
{
	while (true)
	{
		DWORD bytesTransferred = 0;

		CJob* pJob = nullptr;
		OVERLAPPED* pOverlapped = nullptr;

		if (!GetQueuedCompletionStatus(m_jobCompletionPort, &bytesTransferred, (PULONG_PTR)&pJob, &pOverlapped, INFINITE))
		{
			return;
		}

		if (m_shutdown)
		{
			return;
		}

		QueuedJobShPtr pQueuedJob = m_queuedJobs[pJob];

		if (pQueuedJob == nullptr)
		{
			continue;
		}

		int pos = pQueuedJob->m_buffer[0];
		int updatedNumber = m_testData[pos]++;

		ClientSocketShPtr pClientSocket = m_clientSockets[pQueuedJob->m_socket];

		if (pClientSocket == nullptr)
		{
			continue;
		}

		pClientSocket->SetPacketLength(sizeof(updatedNumber));
		memcpy_s(&pClientSocket->m_buffer[0], pClientSocket->m_buffer.size(), &updatedNumber, sizeof(updatedNumber));

		pClientSocket->SetWriteMode();
		pClientSocket->Send();
	}
}

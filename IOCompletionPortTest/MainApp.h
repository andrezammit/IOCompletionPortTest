#pragma once

#include "ClientSocket.h"

#define ClientSocketShPtr	shared_ptr<CClientSocket>
#define ClientSocketMap		concurrent_unordered_map <SOCKET, ClientSocketShPtr>

#define QueuedJobShPtr		shared_ptr<CJob>
#define QueuedJobMap		concurrent_unordered_map<CJob*, QueuedJobShPtr>

#define TestDataMap			concurrent_unordered_map<int, int>

class CJob
{
public:
	CJob() 
	{
		m_socket = INVALID_SOCKET;
	}

	CJob(CClientSocket* pClientSocket)
	{
		m_socket = pClientSocket->m_socket;
		m_buffer = pClientSocket->m_buffer;
	}

	SOCKET m_socket;
	vector<BYTE> m_buffer;
};

class CMainApp
{
public:
	CMainApp();
	~CMainApp();

	void Run();

private:
	SOCKET m_listenSocket;

	bool m_shutdown;

	HANDLE m_jobCompletionPort;
	HANDLE m_socketCompletionPort;

	QueuedJobMap m_queuedJobs;
	ClientSocketMap m_clientSockets;

	TestDataMap m_testData;

	vector<unique_ptr<thread>> m_threads;

	void Uninit();
	void StopThreads();
	void StartThreads();
	void CloseIOCompletionPort();
	void JobProcessingThreadFunc();
	void SocketProcessingThreadFunc();

	void CloseClientSocket(ClientSocketShPtr& pClientSocket);

	static void JobProcessingThread(CMainApp* pMainApp);
	static void SocketProcessingThread(CMainApp* pMainApp);

	bool Init();
	bool StartServer();
	bool CreateIOCompletionPort();

	bool UpdateIOCompletionPort(CClientSocket& clientSocket);
};

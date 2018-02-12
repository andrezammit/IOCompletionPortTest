#pragma once

#include "ClientSocket.h"

#define ClientSocketShPtr	shared_ptr<CClientSocket>
#define ClientSocketMap		concurrent_unordered_map <CClientSocket*, ClientSocketShPtr>

class CMainApp
{
public:
	CMainApp();
	~CMainApp();

	void Run();

private:
	SOCKET m_listenSocket;

	bool m_shutdown;

	HANDLE m_completionPort;

	ClientSocketMap m_clientSockets;
	vector<unique_ptr<thread>> m_threads;

	void Uninit();
	void StopThreads();
	void StartThreads();
	void ProcessingThreadFunc();
	void CloseIOCompletionPort();

	void CloseClientSocket(ClientSocketShPtr& pClientSocket);

	static void ProcessingThread(CMainApp* pMainApp);

	bool Init();
	bool StartServer();
	bool CreateIOCompletionPort();

	bool UpdateIOCompletionPort(CClientSocket& clientSocket);
};

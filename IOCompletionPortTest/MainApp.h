#pragma once

#include "ClientSocket.h"

class CMainApp
{
public:
	CMainApp();
	~CMainApp();

	void Run();

private:
	SOCKET m_listenSocket;

	static bool m_shutdown;

	static HANDLE m_completionPort;

	vector<unique_ptr<thread>> m_threads;
	
	static concurrent_unordered_map <CClientSocket*, shared_ptr<CClientSocket>> m_clientSockets;

	void Uninit();
	void StopThreads();
	void StartThreads();
	void CloseIOCompletionPort();

	static void ProcessThreadFunc();
	
	static void CloseClientSocket(shared_ptr<CClientSocket>& pClientSocket);

	bool Init();
	bool StartServer();
	bool CreateIOCompletionPort();

	bool UpdateIOCompletionPort(CClientSocket& clientSocket);
};

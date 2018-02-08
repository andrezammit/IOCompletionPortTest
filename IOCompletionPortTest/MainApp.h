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

	static HANDLE m_completionPort;

	vector<unique_ptr<thread>> m_threads;
	
	static vector<CClientSocket> m_clientSockets;

	void Uninit();
	void StopThreads();
	void StartThreads();

	static void ProcessThreadFunc();
	
	static void CloseClientSocket(CClientSocket& clientSocket);

	bool Init();
	bool StartServer();
	bool CreateIOCompletionPort();

	bool UpdateIOCompletionPort(CClientSocket& clientSocket);
};

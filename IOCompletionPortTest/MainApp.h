#pragma once

#include "ClientSocket.h"

#define ClientSocketShPtr	shared_ptr<CClientSocket>
#define ClientSocketMap		concurrent_unordered_map <SOCKET, ClientSocketShPtr>

#define TestDataMap			concurrent_unordered_map<int, int>

class CMainApp
{
public:
	CMainApp();
	~CMainApp();

	void Run();

private:
	SOCKET m_listenSocket;

	bool m_shutdown;

	HANDLE m_socketCompletionPort;

	ClientSocketMap m_clientSockets;

	TestDataMap m_testData;

	vector<unique_ptr<thread>> m_threads;

	void Uninit();
	void StopThreads();
	void StartThreads();
	void CloseIOCompletionPort();
	void SocketProcessingThreadFunc();

	void CloseClientSocket(ClientSocketShPtr& pClientSocket);

	static void SocketProcessingThread(CMainApp* pMainApp);

	bool Init();
	bool StartServer();
	bool CreateIOCompletionPort();

	bool ProcessPacket(ClientSocketShPtr& pClientSocket);
	bool UpdateIOCompletionPort(CClientSocket& clientSocket);
};

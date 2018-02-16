#pragma once

class CClientSocket
{
public:
	static const int modeRead = 1;
	static const int modeWrite = 2;

	CClientSocket();
	CClientSocket(SOCKET socket, sockaddr& clientAddress);

	~CClientSocket();

	void SetReadMode();
	void SetWriteMode();
	void ResetTransferredBytes();

	void SetPacketLength(DWORD packetLen);
	void SetClientAddrInfo(sockaddr& clientAddress);
	void AddTransferredBytes(DWORD bytesTransferred);

	bool Send();
	bool IsReadMode();
	bool IsWriteMode();
	bool SendPendingData();
	bool ReceivePendingData();
	bool IsPacketTransferred();
	
	bool Receive(DWORD packetLen);
	bool Send(vector<BYTE>& data);

	USHORT GetPort();

	CString GetClientAddrString();

	DWORD m_transferMode;

	DWORD m_bytesDone;
	DWORD m_packetLen;

	SOCKET m_socket;
	WSAOVERLAPPED m_overlapped;

	vector<BYTE> m_buffer;
	vector<BYTE> m_sockAddr;

	CString m_ipAddr;
};

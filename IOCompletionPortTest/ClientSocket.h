#pragma once

class CClientSocket
{
public:
	static const int modeRead = 1;
	static const int modeWrite = 2;

	CClientSocket();
	~CClientSocket();

	void SetReadMode();
	void SetWriteMode();
	void ResetTransferredBytes();

	void SetPacketLength(DWORD packetLen);
	void AddTransferredBytes(DWORD bytesTransferred);

	bool Send();
	bool Receive();
	bool IsReadMode();
	bool IsWriteMode();
	bool IsPacketTransferred();

	bool operator==(const CClientSocket& rhs);

	DWORD m_transferMode;

	DWORD m_bytesDone;
	DWORD m_packetLen;

	SOCKET m_socket;
	WSAOVERLAPPED m_overlapped;

	vector<BYTE> m_buffer;
};

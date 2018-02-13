#include "stdafx.h"
#include "ClientSocket.h"

#define IPV6_ADDR_LEN	46

CClientSocket::CClientSocket()
{
	m_transferMode = modeRead;

	m_bytesDone = 0;
	m_packetLen = 0;

	m_socket = INVALID_SOCKET;
	ZeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));
}

CClientSocket::CClientSocket(SOCKET socket, sockaddr& clientAddress)
{
	m_transferMode = modeRead;

	m_bytesDone = 0;
	m_packetLen = 0;

	m_socket = socket;
	ZeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));

	SetClientAddrInfo(clientAddress);
}

CClientSocket::~CClientSocket()
{
	closesocket(m_socket);
}

void CClientSocket::SetClientAddrInfo(sockaddr& clientAddress)
{
	void* pAddr = nullptr;

	if (clientAddress.sa_family == AF_INET)
	{
		sockaddr_in* pSockAddrIn = (sockaddr_in*) &clientAddress;
		pAddr = &pSockAddrIn->sin_addr;

		m_sockAddr.resize(sizeof(sockaddr_in));
		memcpy_s(&m_sockAddr[0], m_sockAddr.size(), pSockAddrIn, sizeof(sockaddr_in));
	}
	else
	{
		sockaddr_in6* pSockAddrIn6 = (sockaddr_in6*)&clientAddress;
		pAddr = &pSockAddrIn6->sin6_addr;

		m_sockAddr.resize(sizeof(sockaddr_in6));
		memcpy_s(&m_sockAddr[0], m_sockAddr.size(), pSockAddrIn6, sizeof(sockaddr_in6));
	}

	InetNtop(clientAddress.sa_family, pAddr, m_ipAddr.GetBuffer(IPV6_ADDR_LEN), IPV6_ADDR_LEN);
	m_ipAddr.ReleaseBuffer();
}

USHORT CClientSocket::GetPort()
{
	if (m_sockAddr.size() == 0)
	{
		return 0;
	}

	sockaddr* pSockAddr = (sockaddr*) &m_sockAddr[0];

	if (pSockAddr->sa_family == AF_INET)
	{
		sockaddr_in* pSockAddrIn = (sockaddr_in*)&pSockAddr;
		return pSockAddrIn->sin_port;
	}
	
	sockaddr_in6* pSockAddrIn6 = (sockaddr_in6*)&pSockAddr;
	return pSockAddrIn6->sin6_port;
}

CString CClientSocket::GetClientAddrString()
{
	CString clientAddr;
	clientAddr.Format(_T("%s:%d"), m_ipAddr, GetPort());

	return clientAddr;
}

void CClientSocket::SetPacketLength(DWORD packetLen)
{
	m_packetLen = packetLen;
	m_buffer.resize(packetLen);
}

bool CClientSocket::IsPacketTransferred()
{
	return m_bytesDone == m_packetLen;
}

void CClientSocket::ResetTransferredBytes()
{
	m_bytesDone = 0;
}

void CClientSocket::AddTransferredBytes(DWORD bytesTransferred)
{
	m_bytesDone += bytesTransferred;
}

bool CClientSocket::IsReadMode()
{
	return m_transferMode == modeRead;
}

bool CClientSocket::IsWriteMode()
{
	return m_transferMode == modeWrite;
}

void CClientSocket::SetReadMode()
{
	m_transferMode = modeRead;
}

void CClientSocket::SetWriteMode()
{
	m_transferMode = modeWrite;
}

bool CClientSocket::Receive()
{
	if (!IsReadMode())
	{
		return true;
	}

	if (m_buffer.size() < m_packetLen)
	{
		m_buffer.resize(m_packetLen);
	}

	DWORD bufferPos = m_bytesDone;

	WSABUF dataBuff;

	dataBuff.len = m_buffer.size() - bufferPos;
	dataBuff.buf = (char*)&m_buffer[bufferPos];

	DWORD flags = 0;

	int ret = WSARecv(m_socket, &dataBuff,
		1, NULL, &flags,
		&m_overlapped, NULL);

	if (ret != 0)
	{
		int error = WSAGetLastError();

		if (error != WSA_IO_PENDING)
		{
			return false;
		}
	}

	return true;
}

bool CClientSocket::Send()
{
	if (!IsWriteMode())
	{
		return true;
	}

	if (m_buffer.size() < m_packetLen)
	{
		m_buffer.resize(m_packetLen);
	}

	DWORD bufferPos = m_bytesDone;

	WSABUF dataBuff;

	dataBuff.len = m_buffer.size() - bufferPos;
	dataBuff.buf = (char*)&m_buffer[bufferPos];

	DWORD flags = 0;

	WSASend(m_socket, &dataBuff, 1, NULL, flags, &m_overlapped, NULL);

	return true;
}
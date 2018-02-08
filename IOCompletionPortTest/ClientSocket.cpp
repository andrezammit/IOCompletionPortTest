#include "stdafx.h"
#include "ClientSocket.h"

CClientSocket::CClientSocket()
{
	m_transferMode = modeRead;

	m_bytesDone = 0;
	m_packetLen = 0;

	m_socket = INVALID_SOCKET;
	ZeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));
}

CClientSocket::~CClientSocket()
{
	closesocket(m_socket);
}

bool CClientSocket::operator==(const CClientSocket& rhs)
{
	return this == &rhs;
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
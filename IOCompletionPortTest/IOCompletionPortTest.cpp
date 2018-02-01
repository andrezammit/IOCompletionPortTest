// IOCompletionPortTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <thread>
#include <vector>

using namespace std;

HANDLE m_completionPort = NULL;

class CWorkItem
{
public:
	CWorkItem()
	{
		m_id = ++m_nextId;

		m_counter = 0;
	}

	DWORD m_id;
	DWORD m_counter;

private:
	static DWORD m_nextId;
};

DWORD CWorkItem::m_nextId = 0;

void ProcessThreadFunc()
{
	while (true)
	{
		DWORD bytesWritten = 0;

		CWorkItem* pWorkItem = NULL;
		OVERLAPPED* pOverlapped = NULL;
		
		GetQueuedCompletionStatus(m_completionPort, &bytesWritten, (PULONG_PTR) &pWorkItem, &pOverlapped, INFINITE);
		
		if (pWorkItem != NULL)
		{
			pWorkItem->m_counter++;

			wprintf_s(_T("Thread: %d, ID: %d, counter: %d\n"), GetCurrentThreadId(), pWorkItem->m_id, pWorkItem->m_counter);
		}
	}
}

int main()
{
	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);

	vector<CWorkItem> workItems;
	workItems.resize(5);

	for (int cnt = 0; cnt < 10; cnt++)
	{
		for (auto& workItem : workItems)
		{
			PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)&workItem, NULL);
		}
	}

	thread processThread1(ProcessThreadFunc);
	thread processThread2(ProcessThreadFunc);

	processThread1.join();
	processThread2.join();

    return 0;
}


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <atlstr.h>

#include <ws2tcpip.h>

#include <mutex>
#include <thread>
#include <vector>

#include <concurrent_unordered_map.h>

using namespace std;
using namespace concurrency;

#pragma comment(lib, "Ws2_32.lib")

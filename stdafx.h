// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <SDKDDKVer.h>

#include <stdio.h>
#include <tchar.h>
#include <stdint.h>

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
#include <conio.h>

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <chrono>

#include "asio.hpp"
#include "asio/steady_timer.hpp"




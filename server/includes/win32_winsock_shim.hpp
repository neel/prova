#ifndef WIN32_WINSOCK_SHIM_HPP
#define WIN32_WINSOCK_SHIM_HPP

#ifdef _WIN32
// block winsock.h from windows.h
#define _WINSOCKAPI_
// lean down windows.h
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#endif // WIN32_WINSOCK_SHIM_HPP

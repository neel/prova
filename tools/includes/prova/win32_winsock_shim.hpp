#ifndef WIN32_WINSOCK_SHIM_HPP
#define WIN32_WINSOCK_SHIM_HPP

#ifdef _WIN32
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

#endif

#endif // WIN32_WINSOCK_SHIM_HPP

//#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#endif

#include "serverudp.h"

/*---------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: ServerUDP.cpp
--
-- PROGRAM: ComAudioServer
--
-- FUNCTIONS:
--    bool init_socket(short port);
--    bool init_multicast(const char *name);
--    bool broadcast_message(char *message, LPDWORD lp_bytes_sent);
--
-- DATE: APRIL 14 2016
--
-- REVISIONS: APRIL 14 2016
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- NOTES:
-- Networking functions for multicast UDP.
---------------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------------------
-- FUNCTION: init_socket
--
-- DATE: APRIL 14 2016
--
-- REVISIONS: APRIL 14 2016
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: init_socket(short port)
--              port:       port on which to host socket.
--
-- RETURNS: bool success
--
-- NOTES:
-- Creates the UDP socket.
---------------------------------------------------------------------------------------------------------------------*/
bool ServerUDP::init_socket(short port) {

    int opt = 1;

    // load winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        qDebug() << "WSAStartup() failed: " << WSAGetLastError();
        return false;
    }

    // create udp socket
    if ((sock_info.Socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
        qDebug() << "WSASocket() failed: " << WSAGetLastError();
        return false;
    }

    // set resuse addr
    if (setsockopt(sock_info.Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0)
    {
        qDebug() << "WSASocket() failed: " << WSAGetLastError();
        return false;
    }

    local_addr.sin_family       = AF_INET;
    local_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    local_addr.sin_port         = htons(port);

    if (bind(sock_info.Socket, (LPSOCKADDR) &local_addr, sizeof(local_addr)) == SOCKET_ERROR) {
        qDebug() << "bind() failed: " << WSAGetLastError();
        return false;
    }

    return true;
}

/*---------------------------------------------------------------------------------------------------------------------
-- FUNCTION: init_multicast
--
-- DATE: APRIL 14 2016
--
-- REVISIONS: APRIL 14 2016
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: init_multicast(const char *name)
--              name: the IP subnet mast to host multicast on.
--
-- RETURNS: bool success
--
-- NOTES:
-- Sets the socket options for multicast.
---------------------------------------------------------------------------------------------------------------------*/
bool ServerUDP::init_multicast(const char *name) {
    BOOL loop_back = false;
    u_long time_to_live = 1;

    //TODO replace with local ip address
    multicast_addr.imr_multiaddr.s_addr = inet_addr(name);
    multicast_addr.imr_interface.s_addr = INADDR_ANY;


    if(setsockopt(sock_info.Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&time_to_live, sizeof(time_to_live)) == SOCKET_ERROR)
    {
        qDebug() << "setsockopt() on TTL failed: " << WSAGetLastError();
        return false;
    }

    if(setsockopt(sock_info.Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicast_addr, sizeof(multicast_addr)) == SOCKET_ERROR)
    {
        qDebug() << "setsockopt() on multicast address failed: " << WSAGetLastError();
        return false;
    }

    if(setsockopt(sock_info.Socket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loop_back, sizeof(loop_back)) == SOCKET_ERROR)
    {
        qDebug() << "setsockopt() on disable loopback failed: " << WSAGetLastError();
        return false;
    }

    dest_addr.sin_family       = AF_INET;
    dest_addr.sin_addr.s_addr  = inet_addr(name);
    dest_addr.sin_port         = htons(4985);

    return true;
}

/*---------------------------------------------------------------------------------------------------------------------
-- FUNCTION: broadcast_message
--
-- DATE: APRIL 14 2016
--
-- REVISIONS: APRIL 14 2016
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: broadcast_message(char *message, LPDWORD lp_bytes_sent)
--                  message:        pointer to data
--                  lp_bytes_sent:  amount of data to send
--
-- RETURNS: bool success
--
-- NOTES:
-- Broadcasts data to socket.
---------------------------------------------------------------------------------------------------------------------*/
bool ServerUDP::broadcast_message(char *message, LPDWORD lp_bytes_sent) {
    sock_info.DataBuf.buf = message;
    sock_info.DataBuf.len = *lp_bytes_sent;

    ZeroMemory(&sock_info.Overlapped, sizeof(WSAOVERLAPPED));
    sock_info.Overlapped.hEvent = WSACreateEvent();

    if (WSASendTo(sock_info.Socket,
                  &(sock_info.DataBuf),
                  1, NULL, flags,
                  (SOCKADDR*) &dest_addr,
                  sizeof(dest_addr),
                  &(sock_info.Overlapped), NULL) < 0) {

        if (WSAGetLastError() != WSA_IO_PENDING) {
            qWarning() << "UDP WSASendTo() failed: " << WSAGetLastError();
            return false;
        }

        if (WSAWaitForMultipleEvents(1, &sock_info.Overlapped.hEvent, false, INFINITE, false) == WAIT_TIMEOUT) {
            qWarning() << "UDP WSASendTo() timeout";
            return false;
        }
    }

    if(!WSAGetOverlappedResult(sock_info.Socket, &(sock_info.Overlapped), &sock_info.BytesSEND, FALSE, &flags))
    {
        qWarning() << "UDP WSAGetOverlappedResult() failed: " << WSAGetLastError();
        return false;

    }
    return true;
}






























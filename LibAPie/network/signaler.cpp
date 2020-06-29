/*
    Copyright (c) 2007-2011 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "signaler.h"

#include <assert.h>

#if defined WIN32
#include "../network/windows_platform.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

APie::Signaler::Signaler()
{
    //  Create the socketpair for signaling.
    int rc = makeFdPair(&r, &w);
    assert(rc == 0);

	evutil_make_socket_nonblocking(w);
	evutil_make_socket_nonblocking(r);
}

APie::Signaler::~Signaler()
{
	evutil_closesocket(w);
	evutil_closesocket(r);
}

evutil_socket_t APie::Signaler::getFd()
{
    return r;
}

void APie::Signaler::send ()
{
#if defined WIN32
    unsigned char dummy = 0;
    int nbytes = ::send(w, (char*)&dummy, sizeof(dummy), 0);

	//  If not a single byte can be written to the socket in non-blocking mode
	//  we'll get an error (this may happen during the speculative write).
	if (nbytes == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)
	{
		return;
	}

	int iLastError = WSAGetLastError();
	//  Signalise peer failure.
	if (nbytes == -1 && (
		iLastError == WSAENETDOWN ||
		iLastError == WSAENETRESET ||
		iLastError == WSAEHOSTUNREACH ||
		iLastError == WSAECONNABORTED ||
		iLastError == WSAETIMEDOUT ||
		iLastError == WSAECONNRESET))
	{
		return;
	}

    assert(nbytes != SOCKET_ERROR);
    assert(nbytes == sizeof(dummy));
#else
    unsigned char dummy = 0;
    while (true) 
	{
        ssize_t nbytes = ::send(w, &dummy, sizeof(dummy), 0);
        if (nbytes == -1 && errno == EINTR)
		{
			continue;
		}

		//  Several errors are OK. When speculative write is being done we may not
		//  be able to write a single byte to the socket. Also, SIGSTOP issued
		//  by a debugging tool can result in EINTR error.
		if (nbytes == -1 && (errno == EWOULDBLOCK ||errno == EINTR))
		{
			return;
		}

		//  Signalise peer failure.
		if (nbytes == -1 && (errno == ECONNRESET || errno == EPIPE))
		{
			return;
		}

        //assert(nbytes == sizeof(dummy));
		if (nbytes != sizeof(dummy))
		{
			ASYNC_PIE_LOG("Signaler/send", PIE_CYCLE_DAY, PIE_ERROR, "nbytes:%d|errno:%d",
				nbytes, errno);
		}
        break;
    }
#endif
}

void APie::Signaler::recv()
{
    //  Attempt to read a signal.
    unsigned char dummy;
#ifdef WIN32
    int nbytes = ::recv(r, (char*)&dummy, sizeof(dummy), 0);
	if (nbytes == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)
	{
		return;
	}

	int iLastError = WSAGetLastError();
	//  Connection failure.
	if (nbytes == -1 && (
		iLastError == WSAENETDOWN ||
		iLastError == WSAENETRESET ||
		iLastError == WSAECONNABORTED ||
		iLastError == WSAETIMEDOUT ||
		iLastError == WSAECONNRESET ||
		iLastError == WSAECONNREFUSED ||
		iLastError == WSAENOTCONN))
	{
		return;
	}

    assert(nbytes != SOCKET_ERROR);
#else
    ssize_t nbytes = ::recv(r, &dummy, sizeof(dummy), 0);

	//  Several errors are OK. When speculative read is being done we may not
	//  be able to read a single byte to the socket. Also, SIGSTOP issued
	//  by a debugging tool can result in EINTR error.
	if (nbytes == -1
		&& (errno == EAGAIN
		|| errno == EWOULDBLOCK
		|| errno == EINTR))
	{
		return;
	}

	//  Signal peer failure.
	if (nbytes == -1
		&& (errno == ECONNRESET
		|| errno == ECONNREFUSED
		|| errno == ETIMEDOUT
		|| errno == EHOSTUNREACH
		|| errno == ENOTCONN))
	{
		return;
	}

    assert(nbytes >= 0);
#endif
    assert(nbytes == sizeof (dummy));
    assert(dummy == 0);
}

int APie::Signaler::makeFdPair(evutil_socket_t *r, evutil_socket_t *w)
{
	evutil_socket_t pair[2];

#ifdef WIN32
	int rc = evutil_socketpair(AF_INET, SOCK_STREAM, 0, pair);
#else
	int rc = evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
#endif

	*w = pair[0];
	*r = pair[1];

	return rc;
}


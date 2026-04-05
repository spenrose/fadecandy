/*
 * Plain TCP server for Open Pixel Control protocol
 *
 * Copyright (c) 2013 Micah Elizabeth Scott
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "tcpnetserver.h"
#include "tinythread.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>


TcpNetServer::TcpNetServer(OPC::callback_t opcCallback, void *context, bool verbose)
    : mOpcCallback(opcCallback), mUserContext(context), mVerbose(verbose)
{}

struct OpcServerArgs {
    TcpNetServer *self;
    int listenFd;
};

struct OpcClientArgs {
    TcpNetServer *self;
    int fd;
};

bool TcpNetServer::startOPC(const char *host, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("startOPC: socket");
        return false;
    }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        perror("startOPC: bind");
        close(fd);
        return false;
    }

    if (listen(fd, 8) < 0) {
        perror("startOPC: listen");
        close(fd);
        return false;
    }

    fprintf(stderr, "OPC server listening on %s:%d\n", host ? host : "0.0.0.0", port);

    OpcServerArgs *args = new OpcServerArgs;
    args->self = this;
    args->listenFd = fd;
    new tthread::thread(opcThreadFunc, args);
    return true;
}

void TcpNetServer::opcThreadFunc(void *arg)
{
    OpcServerArgs *sargs = (OpcServerArgs*)arg;
    TcpNetServer *self = sargs->self;
    int listenFd = sargs->listenFd;
    delete sargs;

    for (;;) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof clientAddr;
        int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientFd < 0) {
            perror("startOPC: accept");
            continue;
        }

        int yes = 1;
        setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

        OpcClientArgs *cargs = new OpcClientArgs;
        cargs->self = self;
        cargs->fd = clientFd;
        new tthread::thread(opcClientFunc, cargs);
    }
}

void TcpNetServer::opcClientFunc(void *arg)
{
    OpcClientArgs *cargs = (OpcClientArgs*)arg;
    TcpNetServer *self = cargs->self;
    int fd = cargs->fd;
    delete cargs;

    uint8_t header[OPC::HEADER_BYTES];
    uint8_t *body = NULL;
    size_t bodyAlloc = 0;

    for (;;) {
        // Read the 4-byte OPC header
        ssize_t got = 0;
        while (got < (ssize_t)OPC::HEADER_BYTES) {
            ssize_t n = read(fd, header + got, OPC::HEADER_BYTES - got);
            if (n <= 0) goto done;
            got += n;
        }

        OPC::Message *msg = (OPC::Message*)header;
        size_t bodyLen = msg->length();

        // Grow body buffer if needed
        if (bodyLen > bodyAlloc) {
            free(body);
            body = (uint8_t*)malloc(bodyLen);
            if (!body) goto done;
            bodyAlloc = bodyLen;
        }

        // Read body
        got = 0;
        while (got < (ssize_t)bodyLen) {
            ssize_t n = read(fd, body + got, bodyLen - got);
            if (n <= 0) goto done;
            got += n;
        }

        // Build a complete OPC message and dispatch it
        {
            uint8_t msgBuf[OPC::HEADER_BYTES + bodyLen];
            memcpy(msgBuf, header, OPC::HEADER_BYTES);
            memcpy(msgBuf + OPC::HEADER_BYTES, body, bodyLen);
            self->mOpcCallback(*(OPC::Message*)msgBuf, self->mUserContext);
        }
    }

done:
    free(body);
    close(fd);
}

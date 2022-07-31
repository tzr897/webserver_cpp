#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <assert.h>

#include "locker.h"
#include "cond.h"
#include "sem.h"
#include "threadpool.h"
#include "http_conn.h"
#include "threadpool.cpp"

#define MAX_FD 65536 // maximum number of fd
#define MAX_EVENT_NUMBER 10000 // maximum number of events listened


// definition is in http_conn.cpp
extern void addfd(int epollfd, int fd, bool one_shot); 
extern void removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != 1);
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("usage: %s port_number\n", basename(argv[0]));
        return 1;
    }

    int port = atoi(argv[1]);
    addsig(SIGPIPE, SIG_IGN);

    Threadpool< Http_conn > *pool = NULL;
    try {
        pool = new Threadpool< Http_conn >;
    } catch(...) {
        printf("nonono\n");
        return 1;
    }

    Http_conn *users = new Http_conn[MAX_FD];

    // create the socket
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    int ret;
    struct sockaddr_in address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    // port reuse and binding
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
    ret = listen(listenfd, 5);

    // create epoll and array of events
    epoll_event events[MAX_EVENT_NUMBER] ;
    int epollfd = epoll_create(5);

    // add the listenfd
    addfd(epollfd, listenfd, false);
    Http_conn::m_epollfd = epollfd;

    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        // iterate the array of events
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                // client connecting...
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)(&client_address), &client_addrlength);

                if (connfd < 0) {
                    printf("errno is : %d\n", errno);
                    continue;
                }

                if (Http_conn::m_user_count >= MAX_FD) {
                    // number of connection >= MAX_FD
                    // send a message to the client saying that the server is busy
                    close(connfd);
                    continue;
                }

                // initialize the data of new client, put it into the array
                users[connfd].init(connfd, client_address);

            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // exception or error happens, close the connection
                users[sockfd].close_conn();

            } else if (events[i].events & EPOLLIN) {
                // read() reads all data at one time
                if (users[sockfd].read()) {
                    // put the target pointer in
                    pool->append(users + sockfd);
                } else {
                    // read() fails, close the connection
                    users[sockfd].close_conn();
                }

            } else if (events[i].events & EPOLLOUT) {
                // write() writes all data at one time
                if (!users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;

    return 0;
}


#ifndef __HTTP_CONN__H
#define __HTTP_CONN__H

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>
#include <string.h>

#include "locker.h"
#include "cond.h"
#include "sem.h"


int set_nonblocking(int fd);

// append the fd that needs to be listened into epoll
void addfd(int epollfd, int fd, bool one_shot);

// remove the fd that needs to be listened from epoll
void removefd(int epollfd, int fd);

// modify the fd, reset the EPOLLONESHOT event on socket
// this is to ensure that EPOLLIN event can be triggered on next read()
void modfd(int epollfd, int fd, int ev);

class Http_conn
{
public:
    // maximum length of filename
    static const int FILENAME_LEN = 200; 
    
    // size of the reading buffer
    static const int READ_BUFFER_SIZE = 2048;

    // size of the writing buffer
    static const int WRITE_BUFFER_SIZE = 1024;

    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT};

    // status of FSM
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};

    // results of processing HTTP requests
    enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

    // status of line
    // LINE_OK: get a complete line
    // LINE_BAD: error with this line
    // LINE_OPEN: this line is incomplete
    enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};

public:
    // all events of socket is registered in one epoll
    static int m_epollfd;

    // number of users
    static int m_user_count;

private:
    // fd of socket
    int m_sockfd;

    // socket address of another one
    sockaddr_in m_address;

    // reading buffer
    char m_read_buf[READ_BUFFER_SIZE];
    
    // the next index of the last byte that has been read
    int m_read_idx;

    // the index of current character in the reading buffer
    int m_checked_idx;

    // the starting index of current line
    int m_start_line;

    // the current status of the FSM
    CHECK_STATE m_check_state;

    // the request method
    METHOD m_method;

    // the complete file path of requested file, it is equal to doc_root + m_url
    char m_real_file[FILENAME_LEN];

    // the filename of requested file
    char *m_url;

    // version of HTTP
    char *m_version;

    // hostname
    char *m_host;

    // the total length of HTTP request
    int m_content_length;

    // whether the HTTP request requires keeping connection
    bool m_linger;

    // writing buffer
    char m_write_buf[WRITE_BUFFER_SIZE];

    // number of bytes waiting to be sent in the writing buffer
    int m_write_idx;

    // the initial place in the memory for requested file's mmap()
    char *m_file_address;

    // status of target file
    struct stat m_file_stat;

    // for writing
    struct iovec m_iv[2];

    // number of memory blocks written
    int m_iv_count;

    // number of bytes that will be sent
    int bytes_to_send;

    // number of bytes that have been sent
    int bytes_have_send;

public:
    Http_conn();

    ~Http_conn();

    // initializing new connections
    void init(int sockfd, const sockaddr_in& addr);

    // close the socket connection
    void close_conn();

    // process the request from clients
    void process();

    // non-blocking read
    bool read();

    // non-blocking write
    bool write();

private:
    void init();

    // analyze the HTTP request
    HTTP_CODE process_read();

    // complete the HTTP response
    bool process_write(HTTP_CODE ret);

    // these functions are used by process_read() to analyze the HTTP request
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *texy);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char* get_line();
    LINE_STATUS parse_line();

    // these functions are used by process_write() to complete the HTTP response
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_content_type();
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

};

#endif
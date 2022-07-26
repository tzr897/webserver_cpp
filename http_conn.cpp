#include "http_conn.h"

int set_nonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// append the fd that needs to be listened into epoll
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot) {
        // prevent one connection from being processed by different threads
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // set the fd non-blocking
    set_nonblocking(fd);
}

// remove the fd that needs to be listened from epoll
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// modify the fd, reset the EPOLLONESHOT event on socket
// this is to ensure that EPOLLIN event can be triggered on next read()
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


// total number of clients
int Http_conn::m_user_count = 0;
int Http_conn::m_epollfd = -1;

Http_conn::Http_conn() {}

Http_conn::~Http_conn() {}

char* Http_conn::get_line() {
    return m_read_buf + m_start_line;
}

// close the connection
void Http_conn::close_conn() {
    if (m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        // after closing one connection, decrease the number of clients by 1
        --m_user_count;
    }
}

// initialize the connection and the address of socket
void Http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // port reuse
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // add the sockfd into the epoll
    addfd(m_epollfd, sockfd, true);
    // increase the number of clients by 1
    ++m_user_count;

    init(); // call the init() below
}

// the meaning is different from the init(int sockfd, const sockaddr_in& addr) above
void Http_conn::init() {
    // the initial status is checking the request line
    m_check_state = CHECK_STATE_REQUESTLINE;

    // don't keep the connection alive defaultly
    m_linger = false;

    // default method of request: GET
    m_method = GET;
    
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;

    m_start_line = 0;
    m_checked_idx = 0;
 
    m_read_idx = 0;
    bzero(m_read_buf, READ_BUFFER_SIZE);

    m_write_idx = 0;
    bzero(m_write_buf, WRITE_BUFFER_SIZE); // q

    bzero(m_real_file, FILENAME_LEN);
}

// read the data from client 
// until no data can be read, or the connection is closed by client
bool Http_conn::read() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    
    int bytes_read = 0;
    while (true) {
        // save the data from m_read_buf + m_read_idx
        // the length is READ_BUFFER_SIZE - m_read_idx
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // no data can be read
            }
            return false;

        } else if (bytes_read == 0) {
            return false; // the connection is closed by client

        }
        m_read_idx += bytes_read;
    }
    return true;
}

// analyze one line
Http_conn::LINE_STATUS Http_conn::parse_line() {
    char temp;
    for ( ; m_checked_idx < m_read_idx; ++m_checked_idx) {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r') {
            if (m_checked_idx + 1 == m_read_idx) {
                return LINE_OPEN;
            } else if (m_read_buf[m_checked_idx + 1] == '\n') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\n') {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// analyze the request line
// get: request method, target url, http version
Http_conn::HTTP_CODE Http_conn::parse_request_line(char *text) {
    // GET /index.html HTTP/1.1
    m_url = strpbrk(text, " \t");
    if (!m_url) {
        return BAD_REQUEST;
    }

    // GET\0/index.html HTTP/1.1
    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }

    // /index.html HTTP/1.1
    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    if (strcasecmp(m_version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    /**
     * http://192.168.110.129:10000/index.html
    */
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// analyze the headers of HTTP request
Http_conn::HTTP_CODE Http_conn::parse_headers(char *text) {
    // if it is an empty line, it means headers have been analyzed
    if (text[0] == '\0') {
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp( text, "Host:", 5 ) == 0) {
        text += 5;
        text += strspn( text, " \t" );
        m_host = text;
    } else {
        printf("oop! unknow header %s\n", text);
    }
    return NO_REQUEST;
}








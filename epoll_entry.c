/*
reactor作用：对于应用层提供，只需要关注读的数据在哪里rbuffer，以及读的数据的长度
第二就是wbuffer，要发送的数据在哪里
如何测试性能，你的webserver性能怎么样？工具 - wrk

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <error.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <pthread.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "kvstore.h"

#define ENABLE_HTTP_RESPONSE 1

typedef int (*msg_handler)(char* msg, int length, char *response);
static msg_handler kvs_handler;


// listenfd
// 回调函数，如果listenfd有可读事件（listenfd触发EPOLLIN事件的时候），执行accept_cb
// 
int accept_cb(int fd);

// clientfd
// 如果可读（clientfd触发EPOLLIN事件的时候），执行recv_cb
int recv_cb(int fd);
// 如果可写（clientfd触发EPOLLOUT事件的时候），执行send_cb
int send_cb(int fd);

// 一个连接的信息包含三部分：fd，buffer，callback
// struct conn_item{
//     int fd;

//     char rbuffer[BUFFER_LENGTH];
//     int rlen;
//     char wbuffer[BUFFER_LENGTH];
//     int wlen;

//     char resource[BUFFER_LENGTH]; // abc.html

//     union{
//         RCALLBACK accept_callback;
//         RCALLBACK recv_callback;
//     } recv_t;
//     RCALLBACK send_callback;
// };
// libevent 不支持多线程，原因是epoll和connlist是共用的

int epfd = 0;
// 当分配的数组大于2G的时候会出现错误
// relocation truncated to fit: R_X86_64_PC32 against symbol `tv_start' defined in .bss section in /tmp/ccdqYpgF.o
struct conn_item connlist[1048576] = {0}; // 2 * 512 * 1024 * 1024
struct timeval tv_start;

#if ENABLE_HTTP_RESPONSE

#define ROOT_DIR "/home/lee/0voice2310/2.1.1-multi-io/"

    typedef struct conn_item connection_t;

    // http://192.168.137.128:2024/index.html
    // GET index.html HTTP/1.1

    // http://192.168.137.128:2024/abc.html
    // GET abc.html HTTP/1.1
    int http_request(connection_t *conn){ // 如何通过请求的信息拿到想要的文件
        // conn -> buffer
        // conn -> wlen

        return 0;
    }

    // 
    int http_response(connection_t *conn){
    #if 1
        conn->wlen = sprintf(conn->wbuffer,
            "HTTP/1.1 200 OK\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: 82\r\n"
            "Content-Type: text/html\r\n"
            "Date: Sat, 01 Sep 2020 13:14:21 GMT\r\n\r\n"
            "<html><head><title>0voice.king</title></head><body><h1>king</h1></body></html>\r\n\r\n");
    #else
        int filefd = open(ROOT_DIR "index.html", O_RDONLY);

        struct stat stat_buf;
        fstat(filefd, &stat_buf);

        conn->wlen = sprintf(conn->wbuffer,
            "HTTP/1.1 200 OK\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: %ld\r\n"
            "Content-Type: text/html\r\n"
            "Date: Sat, 01 Sep 2020 13:14:21 GMT\r\n\r\n", stat_buf.st_size);

        int count = read(filefd, conn->wbuffer + conn->wlen, BUFFER_LENGTH - conn->wlen);
        conn->wlen += count;

        // sendfile
    #endif
        return conn->wlen;
    }
#endif

// struct reactor{
//     int epfd;
//     struct conn_item *connlist;
// };

#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000) // 单位：ms

void kvs_request(struct conn_item *item){
    item->wlen = kvs_handler(item->rbuffer, item->rlen, item->wbuffer);
}

int set_event(int fd, int event, int flag){
    // 1 add , 0 mod
    if(flag){
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }
    else{
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }

}

int accept_cb(int fd){
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
    if(clientfd < 0){
        return -1;
    }

    set_event(clientfd, EPOLLIN, 1);

    connlist[clientfd].fd = clientfd;
    memset(connlist[clientfd].rbuffer, 0, BUFFER_LENGTH);
    connlist[clientfd].rlen = 0;
    memset(connlist[clientfd].wbuffer, 0, BUFFER_LENGTH);
    connlist[clientfd].wlen = 0;
    connlist[clientfd].recv_t.recv_callback = recv_cb;
    connlist[clientfd].send_callback = send_cb;

    if((clientfd % 1000) == 999){
        struct timeval tv_cur;
        gettimeofday(&tv_cur, NULL);
        int time_used = TIME_SUB_MS(tv_cur, tv_start);

        memcpy(&tv_start, &tv_cur, sizeof(struct timeval));

        printf("clientfd:%d, time_used:%d\n", clientfd, time_used);
    }

    return clientfd;
}

// 作用：接收数据；将接收的buffer放到合适的位置
int recv_cb(int fd){ // fd --> EPOLLIN
    char *buffer = connlist[fd].rbuffer;
    int idx = connlist[fd].rlen;

    int count = recv(fd, buffer, BUFFER_LENGTH, 0);
    if(count == 0){
        printf("disconnect\n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return -1;
    }
    connlist[fd].rlen += count;

#if 0 // 要发送的数据
    memcpy(connlist[fd].wbuffer, connlist[fd].rbuffer, connlist[fd].rlen);

    connlist[fd].wlen = connlist[fd].rlen;
    connlist[fd].rlen -= connlist[fd].rlen;
#else
    // http_request(&connlist[fd]);
    // http_response(&connlist[fd]);
    // kvstore_request(&connlist[fd]);
    // connlist[fd].wlen = strlen(connlist[fd].wbuffer);

    kvs_request(&connlist[fd]);
    
#endif

    // send(fd, buffer, idx, 0); 直接send，和设置事件优缺点是什么？

    // 设置事件
    set_event(fd, EPOLLOUT, 0);

    return count;
}

int send_cb(int fd){
    char *buffer = connlist[fd].wbuffer;
    int idx = connlist[fd].wlen;

    int count = send(fd, buffer, idx, 0);

    /*
    发送完之后，fd依然监听 EPOLLOUT，
    由于 TCP socket 绝大多数时间都是可写的，程序就会疯狂调用send_cb(),
    所以发送完成后，应该重新切换
    */ 
    set_event(fd, EPOLLIN, 0);

    return count;
}

int init_server(unsigned short port){
    // 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 绑定ip地址
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));

    // 把服务端用于通讯的IP和端口绑定到socket上
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 端口1024前面的端口号是系统在用如果要用的话要用root权限
    serveraddr.sin_port = htons(port);

    if(-1 == bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr))){
        perror("bind");
        return -1;
    }

    listen(sockfd, 10);

    // 返回的是已经listen的fd
    return sockfd;
}

// tcpserver
int epoll_entry(msg_handler handler){
    int port_count = 20;
    unsigned short port = 2048;
    int i = 0;

    kvs_handler = handler;

    epfd = epoll_create(1);

    for(i = 0; i < port_count; i++){
        int sockfd = init_server(port + i);
        //
        connlist[sockfd].fd = sockfd;
        connlist[sockfd].recv_t.accept_callback = accept_cb;
        set_event(sockfd, EPOLLIN, 1);
    }

    gettimeofday(&tv_start, NULL);

    // pthread_create();

    /*
    struct epoll_event
    {
        uint32_t events;	// Epoll events,对应的事件，传进去的事件
        epoll_data_t data;	// User data variable，可用参数
    } __EPOLL_PACKED;
    */

    struct epoll_event events[1024] = {0};
    while(1){ // mainloop
        int nready = epoll_wait(epfd, events, 1024, -1);

        for(int i = 0; i < nready; i++){
            int connfd = events[i].data.fd;
            /*
            if(sockfd == connfd){
                // accept_cb()
                // int clientfd = accept_cb(connfd);

                int clientfd = connlist[sockfd].accept_callback(sockfd);
                printf("clientfd:%d\n", clientfd);
            } 
            else  */
            if(events[i].events & EPOLLIN){
                // recv_cb()
                // int count = recv_cb(connfd);

                int count = connlist[connfd].recv_t.recv_callback(connfd);
                // printf("recv <-- buffer: %s\n", connlist[connfd].rbuffer);
            }
            else if(events[i].events & EPOLLOUT){ 
                //int count = send_cb(connfd);
                // printf("send --> buffer: %s\n", connlist[connfd].wbuffer);
                int count = connlist[connfd].send_callback(connfd);
            }
        }
    }

    // close(clientfd);


    // 防止代码直接退出
    getchar();
}

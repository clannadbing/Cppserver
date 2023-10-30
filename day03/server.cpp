#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include "util.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setnonblocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK); // fcntl(fd, F_GETFL) | O_NONBLOCK) -> 获取文件的flags; fcntl(fd,F_SETFL,flags) -> 设置文件的flags;
}

int main() {
    // 1. 创建一个套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");

    // 2. 设置地址族、IP地址和端口
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr)); // 确定对象被使用前已先被初始化。如果不清空，使用gdb调试器查看addr内的变量，会是一些随机值，未来可能会导致意想不到的问题。
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8889);

    // 3. 将socket地址与文件描述符绑定
    errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");
    
    // 4. 监听这个socket端口
    errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");  // 系统建议的最大值SOMAXCONN被定义为128
    
    // 5. 创建一个epoll文件描述符, 为多个客户端提供服务
    int epfd = epoll_create1(0);
    errif(epfd == -1, "epoll create error");

    // 6. 创建用于存储发生fd发生事件数组, 将sockfd添加到epoll
    struct epoll_event events[MAX_EVENTS], ev; // events[MAX_EVENTS] -> 用于存储发生fd发生事件，ev -> 将fd添加到epoll
    bzero(&events, sizeof(events));
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET; // 该代码中使用了ET模式，且未处理错误
    ev.data.fd = sockfd; // 该IO口为服务器socket fd
    setnonblocking(sockfd); // 设置文件描述符非阻塞, read()函数非阻塞
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev); // 将服务器socket fd添加到epoll

    // 7. 不断监听epoll上的事件并处理
    while (true) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1); // 有nfds个fd发生事件
        errif(nfds == -1, "epoll wait error");
        for (int i = 0; i < nfds; i++) { // 处理这nfds个事件
            if (events[i].data.fd == sockfd) { // 发生事件的fd是服务器socket fd，表示有新客户端连接
                // 7.1 接受客户端连接，保存客户端的socket地址信息
                struct sockaddr_in clnt_addr;
                socklen_t clnt_addr_len = sizeof(clnt_addr);
                bzero(&clnt_addr, sizeof(clnt_addr));
                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                errif(clnt_sockfd == -1, "socket accept error");
                printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
                // 7.2 将该客户端的socket fd添加到epoll
                bzero(&ev, sizeof(ev));
                ev.data.fd = clnt_sockfd;
                ev.events = EPOLLIN | EPOLLET;
                setnonblocking(clnt_sockfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev);
            }
            else if(events[i].events & EPOLLIN) { // 可读事件
                char buf[READ_BUFFER];
                while (true) { //由于使用非阻塞IO，需要不断读取，直到全部读取完毕
                    ssize_t bytes_read = read(events[i].data.fd, buf, sizeof(buf));
                    if (bytes_read > 0) {
                        printf("message from client fd %d: %s\n", events[i].data.fd, buf);
                        write(events[i].data.fd, buf, sizeof(buf));
                    }
                    else if (bytes_read == -1 && errno == EINTR) { // 客户端正常中断、继续读取
                        printf("continue reading");
                        continue;
                    }
                    else if (bytes_read == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) { //非阻塞IO，这个条件表示数据全部读取完毕
                        printf("finish reading once, errno: %d\n", errno);
                        break;
                    }
                    else if (bytes_read == 0) { //EOF, 客户端断开连接
                        printf("EOF, client fd %d disconnected\n", events[i].data.fd);
                        close(events[i].data.fd); // 关闭socket会自动将文件描述符从epoll树上移除
                        break;
                    }
                }
            }
            else { // 其他事件，之后的版本实现
                    printf("something else happened\n");
            }
        }
    }
    close(sockfd);
    return 0;
}

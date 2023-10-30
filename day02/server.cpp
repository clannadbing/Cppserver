#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
using namespace std;
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
    
    // 5. 接受客户端连接，保存客户端的socket地址信息
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_len = sizeof(clnt_addr);
    bzero(&clnt_addr, sizeof(clnt_addr));
    int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
    errif(clnt_sockfd == -1, "socket accept error");
    printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
    
    // 6. 数据读写操作
    while (true) {
        char buf[1024]; // 定义缓冲区
        bzero(&buf, sizeof(buf)); // 清空缓冲区
        ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf)); // 从客户端socket的读缓存中读取数据到缓冲区中，返回已读数据大小
        if (read_bytes > 0) {
            printf("message from client fd %d: %s\n", clnt_sockfd, buf);
            write(clnt_sockfd, buf, sizeof(buf));
        }
        else if (read_bytes == 0) { // read返回0, 表示EOF
            printf("client fd %d disconnect\n", clnt_sockfd);
            close(clnt_sockfd);
            break;
        }
        else if (read_bytes == -1) {// read返回-1, 表示发生错误
            close(clnt_sockfd);
            errif(true, "socket read error");
        }
    }
    close(sockfd);
    return 0;
}

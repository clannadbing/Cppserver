#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
using namespace std;
int main() {
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1, "socket create error");

    // 2. 设置地址族、IP地址和端口
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8891);

    // 3. 连接服务器
    errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");

    // 4. 数据读写操作
    while (true) {
        char buf[1024]; // 定义缓冲区
        bzero(&buf, sizeof(buf)); // 清空缓冲区
        scanf("%s", buf); // 从键盘输入要传到服务器的数据
        ssize_t write_bytes = write(sockfd, buf, sizeof(buf));
        if (write_bytes == -1) { // write返回-1, 表示发生错误, 
            printf("socket already disconnected, can't write any more!\n");
            break;
        }
        bzero(&buf, sizeof(buf)); // 清空缓冲区
        ssize_t read_bytes = read(sockfd, buf, sizeof(buf)); // 从服务器socket的读缓存中读取数据到缓冲区中，返回已读数据大小
        if (read_bytes > 0) {
            printf("message from server fd %d: %s\n", sockfd, buf);
        }
        else if (read_bytes == 0) { // read返回0, 表示EOF
            printf("server fd %d disconnect\n", sockfd);
            break;
        }
        else if (read_bytes == -1) {// read返回-1, 表示发生错误
            close(sockfd);
            errif(true, "socket read error");
        }
    }
    close(sockfd);
    return 0;
}
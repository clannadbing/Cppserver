#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
using namespace std;
int main() {
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 2. 设置地址族、IP地址和端口
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8888);
    // 3. 连接服务器
    connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    return 0;
}
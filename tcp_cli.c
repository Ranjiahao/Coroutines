#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    char buf[1024] = { 0 };
    while (1) {
        printf("send: ");
        scanf("%s", buf);
        send(fd, buf, strlen(buf), 0);
        memset(buf, 0, sizeof(buf));
        int ret = recv(fd, buf, 1024, 0);
        if(ret <= 0) {
            break;
        }
        printf("recv: %s\n", buf);
    }
    close(fd);
}

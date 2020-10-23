#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "coroutine.h"

int tcp_init() {
    int lfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lfd < 0) {
        perror("socket");
        return -1;
    }
    int op = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int r = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        perror("bind");
        return -1;
    }
    listen(lfd, SOMAXCONN);
    if (r < 0) {
        perror("listen");
        return -1;
    }
    return lfd;
}

void set_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag < 0) {
        perror("fcntl");
        return;
    }
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (flag < 0) {
        perror("fcntl");
        return;
    }
}

int epoll_init() {
    int epfd = epoll_create(CORSZ);
    if (epfd < 0) {
        perror("epoll_create");
        return -1;
    }
    return epfd;
}

int epoll_add(int epfd, int fd) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        perror("epoll_ctl ADD");
        return -1;
    }
    return 0;
}

void accept_conn(int lfd, schedule_t* s, int co_ids[], void* (*call_back)(schedule_t*, void*)) {
    int epfd  = epoll_init();
    epoll_add(epfd, lfd);
    struct epoll_event* evs = (struct epoll_event*)malloc(CORSZ * sizeof(struct epoll_event));
    while (1) {
        int nfds = epoll_wait(epfd, evs, CORSZ, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            return;
        }
        int i;
        for (i = 0; i < nfds; ++i) {
            if (evs[i].data.fd == lfd) {
                // 就绪的描述符是监听套接字
                int cfd = accept(lfd, NULL, NULL);
                if (cfd < 0) {
                    perror("accept");
                }
                set_nonblock(cfd);
                int* args = (int*)malloc(2 * sizeof(int));
                args[0] = cfd;
                args[1] = epfd;
                int id = coroutine_creat(s, call_back, args);
                int j;
                for (j = 0; j < CORSZ; ++j) {
                    if (co_ids[j] == -1) {
                        co_ids[j] = id;
                        break;
                    }
                }
                if (j == CORSZ) {
                    printf("连接太多\n");
                }
                epoll_add(epfd, cfd);
                coroutine_running(s, id);
            } else {
                // 就绪的描述符是通信套接字
                int cfd = evs[i].data.fd;
                int j;
                for (j = 0; j <= s->max_id; ++j) {
                    int cid = co_ids[j];
                    if (cid != -1 && cfd == ((int*)(s->coroutines[cid]->args))[0]) {
                        coroutine_resume(s, cid);
                        break;
                    }
                }
            }
        }
        memset(evs, 0, CORSZ * sizeof(struct epoll_event));
    }
    free(evs);
    close(epfd);
}

void* handle(schedule_t* s, void* args) {
    int* arr = (int*)args;
    int cfd = arr[0];
    int epfd = arr[1];
    char buf[1024] = { 0 };
    while (1) {
        int r = read(cfd, buf, 1023);
        if (r == -1) {
            coroutine_yield(s);
        } else if (r == 0) {
            break;
        } else {
            printf("recv: %s\n", buf);
            if (strncasecmp(buf, "exit", 4) == 0) {
                break;
            }
            write(cfd, buf, r);
            memset(buf, 0, sizeof(buf));
        }
    }
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
    if (ret < 0) {
        perror("epoll_ctl EDL");
    }
    free(arr);
    close(cfd);
    printf("peer shutdown");
    return NULL;
}

int main() {
    int lfd = tcp_init();
    set_nonblock(lfd);
    schedule_t *s = schedule_creat();
    int* co_ids = (int*)malloc(CORSZ * sizeof(int));
    int i;
    for (i = 0; i < CORSZ; ++i) {
        co_ids[i] = -1;
    }
    accept_conn(lfd, s, co_ids, handle);
    free(co_ids);
    close(lfd);
    schedule_destroy(s);
}

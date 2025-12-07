#include "socket-info.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

static struct fuse_socket_info global_si;

void fuse_socket_info_init(struct fuse_session *se)
{
    char *env = getenv("FUSE_ENABLE_SOCKET");
    if (!env || !*env)
        return;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // OS chooses port
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(sockfd); return; }
    if (listen(sockfd, 1) < 0) { close(sockfd); return; }

    socklen_t len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0) { close(sockfd); return; }

    snprintf(global_si.filename, sizeof(global_si.filename),
             ".socket:%s:%d", getenv("HOSTNAME") ? getenv("HOSTNAME") : "localhost", getpid());
    snprintf(global_si.content, sizeof(global_si.content),
             "%s:%d:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), getpid());
    global_si.sockfd = sockfd;
}

void fuse_socket_info_destroy(struct fuse_session *se)
{
    if (global_si.sockfd > 0) {
        close(global_si.sockfd);
        global_si.sockfd = 0;
    }
}

struct fuse_socket_info *fuse_session_get_socket_info(struct fuse_session *se)
{
    if (global_si.sockfd > 0) return &global_si;
    return NULL;
}

#include "fuse_kernel.h"
#include "fuse_socket.h"
#include "fuse_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CLIENTS 16
#define BUFFER_SIZE 256

static int server_fd = -1;
static int clients[MAX_CLIENTS];
static int client_count = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static char socket_file_content[BUFFER_SIZE];
struct fuse_dirent socket_file[11];

static void *accept_clients(void *arg) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client < 0) continue;

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = client;
        } else {
            close(client);
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

void fuse_report_socket(int flags) {
  fuse_log(FUSE_LOG_INFO,"Socket is %s\n",socket_file_content);
}
    
int fuse_socket_init(void) {
    const char *env = getenv("FUSE_ENABLE_SOCKET");
    if (!env) return 0;
    int mode = atoi(env);
    if (!mode) return 0;

    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    pid_t pid = getpid();

    snprintf(socket_file->name, 10*sizeof(*socket_file), ".socket:%s:%d", hostname, pid);

    socket_file->namelen=strlen(socket_file->name);
    socket_file->ino    =(int)socket_file;
    
    ((struct dirent *)socket_file)->d_type = DT_REG; // regular file
 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return;
    listen(server_fd, 5);

    socklen_t addrlen = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &addrlen);

    snprintf(socket_file_content, sizeof(socket_file_content), "%s:%d",
             inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    pthread_t tid;
    pthread_create(&tid, NULL, accept_clients, NULL);
    pthread_detach(tid);

    return mode;
}

void fuse_socket_notify(const char *change, pid_t pid, const char *path) {
    char msg[BUFFER_SIZE];
    int len = snprintf(msg, sizeof(msg), "%s:%d:%s\n", change, (int)pid, path);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        send(clients[i], msg, len, 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void fuse_socket_cleanup(void) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        close(clients[i]);
    }
    client_count = 0;
    pthread_mutex_unlock(&clients_mutex);

    if (server_fd >= 0) close(server_fd);
}

int fuse_socket_isfile(const char *name) {
    return strcmp(name, socket_file->name) == 0;
}

int fuse_socket_read(char *buf, size_t size, off_t offset) {
    size_t len = strlen(socket_file_content);
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    memcpy(buf, socket_file_content + offset, size);
    return size;
}

const char *fuse_socket_getname(void) {
    return socket_file->name[0] ? socket_file->name : NULL;
}

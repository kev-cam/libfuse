#ifndef FUSE_SOCKET_INFO_H
#define FUSE_SOCKET_INFO_H

struct fuse_session;

struct fuse_socket_info {
    int sockfd;
    char filename[64];
    char content[128];
};

void fuse_socket_info_init(struct fuse_session *se);
void fuse_socket_info_destroy(struct fuse_session *se);
struct fuse_socket_info *fuse_session_get_socket_info(struct fuse_session *se);

#endif

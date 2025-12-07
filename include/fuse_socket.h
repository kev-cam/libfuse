#ifndef FUSE_SOCKET_H
#define FUSE_SOCKET_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int fuse_socket_init(void);
void fuse_report_socket(int flags);
void fuse_socket_notify(const char *change, pid_t pid, const char *path);
void fuse_socket_notify_ino(const char *change, pid_t pid, const fuse_ino_t nodeid);
void fuse_socket_cleanup(void);
int fuse_socket_isfile(const char *name);
int fuse_socket_read(char *buf, size_t size, off_t offset);
const char *fuse_socket_getname(void);

extern struct fuse_dirent socket_file[];

#ifdef __cplusplus
}
#endif

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

int fuse_socket_client(const char *spec)
{
    // spec is "host:port"
    // we parse into host + port but without malloc
    char host[128];
    int port = 0;

    // Find the colon
    const char *colon = strchr(spec, ':');
    if (!colon) {
        fprintf(stderr, "Invalid socket spec: %s\n", spec);
        return -1;
    }

    int hostlen = colon - spec;
    if (hostlen >= (int)sizeof(host))
        hostlen = sizeof(host) - 1;

    memcpy(host, spec, hostlen);
    host[hostlen] = '\0';

    port = atoi(colon + 1);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port in spec: %s\n", spec);
        return -1;
    }

    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // Resolve hostname or IP
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // First try dotted quad
    if (inet_aton(host, &addr.sin_addr) == 0) {
        // Try hostname lookup
        struct hostent *he = gethostbyname(host);
        if (!he) {
            fprintf(stderr, "Cannot resolve host: %s\n", host);
            close(fd);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Connect
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    // At this point you have a connected client socket
    return fd;
}

// --- Example usage ---
int main(int argc, const char **argv)
{
    const char *spec = argv[1];  // replace with socket_file_content
    int fd = fuse_socket_client(spec);

    if (fd >= 0) {
        printf("Connected!\n");
        // Example: send a test message
        write(fd, "hello\n", 6);

        // Example: read reply
        char buf[256];
        int n = read(fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = 0;
            printf("Received: %s\n", buf);
        }

        close(fd);
    }

    return 0;
}

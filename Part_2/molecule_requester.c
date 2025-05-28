#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_host> <port>\n", argv[0]);
        exit(1);
    }

    const char *server_host = argv[1];
    const char *port_str = argv[2];
    int sockfd;
    struct addrinfo hints, *res, *rp;
    char buffer[BUF_SIZE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo(server_host, port_str, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd != -1) break;
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not create socket\n");
        freeaddrinfo(res);
        exit(1);
    }

    printf("Connected to molecule supplier.\n");
    printf("Enter command (example: DELIVER WATER 3), or -1 to quit: ");

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strcmp(buffer, "-1\n") == 0) break;

        sendto(sockfd, buffer, strlen(buffer), 0, rp->ai_addr, rp->ai_addrlen);

        memset(buffer, 0, BUF_SIZE);
        socklen_t addrlen = rp->ai_addrlen;
        ssize_t len = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, rp->ai_addr, &addrlen);
        if (len > 0) {
            buffer[len] = 0;
            printf("Server: %s", buffer);
        }

        if (!feof(stdin)) printf("Enter command (example: DELIVER WATER 3), or -1 to quit: ");
    }

    freeaddrinfo(res);
    close(sockfd);
    return 0;
}
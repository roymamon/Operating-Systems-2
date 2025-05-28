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
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(server_host, port_str, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sockfd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        freeaddrinfo(res);
        exit(1);
    }
    freeaddrinfo(res);

    printf("connected to molecule supplier.\n");
    printf("enter command (example: ADD OXYGEN 5), or -1 to quit: ");

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strcmp(buffer, "-1\n") == 0) {
            printf("exiting\n");
            break;
        }
        send(sockfd, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUF_SIZE);
        ssize_t len = recv(sockfd, buffer, BUF_SIZE - 1, 0);
        if (len > 0) {
            buffer[len] = 0;
            printf("server: %s", buffer);
        }
        if (!feof(stdin)) {
            printf("enter command (example: ADD OXYGEN 5), or -1 to quit: ");
        }
    }
    close(sockfd);
    return 0;
}

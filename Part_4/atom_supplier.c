#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    char *host = NULL;
    char port_str[16] = {0};

    struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': host = optarg; break;
            case 'p': strncpy(port_str, optarg, sizeof(port_str) - 1); break;
            default:
                fprintf(stderr, "Usage: %s -h <host> -p <port>\n", argv[0]);
                exit(1);
        }
    }

    if (!host || port_str[0] == '\0') {
        fprintf(stderr, "Error: --host and --port are required.\n");
        exit(1);
    }

    struct addrinfo hints, *res, *rp;
    int sockfd;
    char buffer[BUF_SIZE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    int err = getaddrinfo(host, port_str, &hints, &res);
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

    freeaddrinfo(res);

    if (rp == NULL) {
        fprintf(stderr, "Could not connect to %s:%s\n", host, port_str);
        exit(1);
    }

    printf("connected to drinks bar.\n");
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
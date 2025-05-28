#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    char *host = NULL;
    int port = -1;

    struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -h <host> -p <port>\n", argv[0]);
                exit(1);
        }
    }

    if (!host || port == -1) {
        fprintf(stderr, "Error: --host and --port are required.\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
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
        recv(sockfd, buffer, BUF_SIZE - 1, 0);
        printf("server: %s", buffer);

        if (!feof(stdin)) {
            printf("enter command (example: ADD OXYGEN 5), or -1 to quit: ");
        }
    }

    close(sockfd);
    return 0;
}

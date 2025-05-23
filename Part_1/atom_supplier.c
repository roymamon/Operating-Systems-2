#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

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
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("connected to atom warehouse.\n");

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5555
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to atom warehouse.\n");

    while (1) {
        printf("Enter command (e.g. ADD OXYGEN 5): ");
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        send(sockfd, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUF_SIZE);
        recv(sockfd, buffer, BUF_SIZE - 1, 0);
        printf("Server: %s", buffer);
    }

    close(sockfd);
    return 0;
}
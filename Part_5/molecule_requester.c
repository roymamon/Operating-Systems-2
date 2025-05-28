#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    const char *server_ip = NULL;
    int port = -1;

    struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': server_ip = optarg; break;
            case 'p': port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -h <host> -p <port>\n", argv[0]);
                exit(1);
        }
    }

    if (!server_ip || port == -1) {
        fprintf(stderr, "Error: both --host and --port must be provided.\n");
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    char buffer[BUF_SIZE];

    printf("Connected to drinks bar.\n");
    printf("Enter command (example: DELIVER WATER 3), or -1 to quit: ");

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strcmp(buffer, "-1\n") == 0) {
            printf("Exiting.\n");
            break;
        }

        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        memset(buffer, 0, BUF_SIZE);
        socklen_t addrlen = sizeof(server_addr);
        ssize_t len = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0,
                               (struct sockaddr *)&server_addr, &addrlen);
        if (len > 0) {
            buffer[len] = 0;
            printf("Server: %s", buffer);
        }

        if (!feof(stdin)) {
            printf("Enter command (example: DELIVER WATER 3), or -1 to quit: ");
        }
    }

    close(sockfd);
    return 0;
}

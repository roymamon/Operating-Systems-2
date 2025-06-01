#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <getopt.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    const char *host = NULL;
    const char *uds_path = NULL;
    int port = -1;

    struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:p:f:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'f': uds_path = optarg; break;
            default:
                fprintf(stderr, "Usage: %s [-h <host> -p <port>] | [-f <uds_path>]\n", argv[0]);
                exit(1);
        }
    }

    if ((host && uds_path) || (port != -1 && uds_path)) {
        fprintf(stderr, "Error: conflicting arguments. Use either -h/-p or -f, not both.\n");
        exit(1);
    }

    if (!uds_path && (!host || port == -1)) {
        fprintf(stderr, "Error: provide either -h/-p or -f.\n");
        exit(1);
    }

    int sockfd;
    char buffer[BUF_SIZE];

    if (uds_path) {
        // --- UNIX DATAGRAM SOCKET ---
        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_un server_addr = {0};
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, uds_path, sizeof(server_addr.sun_path) - 1);

        struct sockaddr_un local_addr = {0};
        local_addr.sun_family = AF_UNIX;
        snprintf(local_addr.sun_path, sizeof(local_addr.sun_path), "/tmp/molecule_requester_%d.sock", getpid());
        unlink(local_addr.sun_path);
        if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            perror("bind");
            exit(1);
        }

        printf("Connected to drinks bar via UDS.\n");
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

        unlink(local_addr.sun_path);
    } else {
        // --- UDP SOCKET ---
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host);

        printf("Connected to drinks bar via UDP.\n");
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
    }

    close(sockfd);
    return 0;
}
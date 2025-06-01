#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <getopt.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    char *host = NULL;
    int port = -1;
    char *socket_path = NULL;

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
            case 'f': socket_path = optarg; break;
            default:
                fprintf(stderr, "Usage: %s [-h <host> -p <port>] | [-f <socket_path>]\n", argv[0]);
                exit(1);
        }
    }

    if ((socket_path && (host || port != -1)) || (!socket_path && (!host || port == -1))) {
        fprintf(stderr, "Error: Either use -f <socket_path> or both -h <host> and -p <port>, not both.\n");
        exit(1);
    }

    int sockfd;
    char buffer[BUF_SIZE];

    if (socket_path) {
        struct sockaddr_un server_addr_un;
        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) { perror("socket"); exit(1); }

        memset(&server_addr_un, 0, sizeof(server_addr_un));
        server_addr_un.sun_family = AF_UNIX;
        strncpy(server_addr_un.sun_path, socket_path, sizeof(server_addr_un.sun_path) - 1);

        if (connect(sockfd, (struct sockaddr*)&server_addr_un, sizeof(server_addr_un)) < 0) {
            perror("connect");
            exit(1);
        }

        printf("connected to drinks bar via UNIX socket.\n");
    } else {
        struct sockaddr_in server_addr_in;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) { perror("socket"); exit(1); }

        server_addr_in.sin_family = AF_INET;
        server_addr_in.sin_port = htons(port);
        server_addr_in.sin_addr.s_addr = inet_addr(host);

        if (connect(sockfd, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in)) < 0) {
            perror("connect");
            exit(1);
        }

        printf("connected to drinks bar via TCP.\n");
    }

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

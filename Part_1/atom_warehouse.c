#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 1024
#define MAX_ATOMS 1000000000000000000ULL  // 10^18

typedef struct {
    unsigned long long carbon;
    unsigned long long hydrogen;
    unsigned long long oxygen;
} Inventory;

void update_inventory(int client_fd, Inventory* inv, const char* atom, unsigned long long amount) {
    char msg[BUF_SIZE];

    if (strcmp(atom, "CARBON") == 0) {
        if (amount > MAX_ATOMS - inv->carbon) {
            snprintf(msg, sizeof(msg), "error: exceeding storage limit.\n");
            send(client_fd, msg, strlen(msg), 0);
            return;
        }
        inv->carbon += amount;
    } else if (strcmp(atom, "HYDROGEN") == 0) {
        if (amount > MAX_ATOMS - inv->hydrogen) {
            snprintf(msg, sizeof(msg), "error: exceeding storage limit.\n");
            send(client_fd, msg, strlen(msg), 0);
            return;
        }
        inv->hydrogen += amount;
    } else if (strcmp(atom, "OXYGEN") == 0) {
        if (amount > MAX_ATOMS - inv->oxygen) {
            snprintf(msg, sizeof(msg), "error: exceeding storage limit.\n");
            send(client_fd, msg, strlen(msg), 0);
            return;
        }
        inv->oxygen += amount;
    }

    snprintf(msg, sizeof(msg),
             "inventory: CARBON=%llu, HYDROGEN=%llu, OXYGEN=%llu\n",
             inv->carbon, inv->hydrogen, inv->oxygen);
    send(client_fd, msg, strlen(msg), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int server_fd;
    struct sockaddr_in address;
    char buffer[BUF_SIZE];
    Inventory inv = {0};
    fd_set master_set, read_fds;
    int fdmax;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(server_fd, SOMAXCONN);
    printf("atom warehouse server listening on port %d\n", port);

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    fdmax = server_fd;

    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    socklen_t addrlen = sizeof(address);
                    int new_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(new_fd, &master_set);
                        if (new_fd > fdmax) fdmax = new_fd;
                        printf("new client connected.\n");
                    }
                } else {
                    memset(buffer, 0, BUF_SIZE);
                    ssize_t len = recv(i, buffer, BUF_SIZE - 1, 0);
                    if (len <= 0) {
                        close(i);
                        FD_CLR(i, &master_set);
                        printf("client disconnected.\n");
                    } else {
                        char cmd[16], atom[16];
                        unsigned long long amount;
                        if (sscanf(buffer, "%15s %15s %llu", cmd, atom, &amount) == 3 && strcmp(cmd, "ADD") == 0) {
                            update_inventory(i, &inv, atom, amount);
                        } else {
                            char* err = "invalid command. Use: ADD ATOM_NAME AMOUNT\n";
                            send(i, err, strlen(err), 0);
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define MAX_ATOMS 1000000000000000000ULL

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

int can_deliver(const char* molecule, unsigned long long count, Inventory* inv) {
    if (strcmp(molecule, "WATER") == 0) {
        return inv->hydrogen >= 2 * count && inv->oxygen >= count;
    } else if (strcmp(molecule, "CARBON_DIOXIDE") == 0) {
        return inv->carbon >= count && inv->oxygen >= 2 * count;
    } else if (strcmp(molecule, "GLUCOSE") == 0) {
        return inv->carbon >= 6 * count && inv->hydrogen >= 12 * count && inv->oxygen >= 6 * count;
    } else if (strcmp(molecule, "ALCOHOL") == 0) {
        return inv->carbon >= 2 * count && inv->hydrogen >= 6 * count && inv->oxygen >= count;
    }
    return 0;
}

void do_delivery(const char* molecule, unsigned long long count, Inventory* inv) {
    if (strcmp(molecule, "WATER") == 0) {
        inv->hydrogen -= 2 * count;
        inv->oxygen -= count;
    } else if (strcmp(molecule, "CARBON_DIOXIDE") == 0) {
        inv->carbon -= count;
        inv->oxygen -= 2 * count;
    } else if (strcmp(molecule, "GLUCOSE") == 0) {
        inv->carbon -= 6 * count;
        inv->hydrogen -= 12 * count;
        inv->oxygen -= 6 * count;
    } else if (strcmp(molecule, "ALCOHOL") == 0) {
        inv->carbon -= 2 * count;
        inv->hydrogen -= 6 * count;
        inv->oxygen -= count;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <tcp_port> <udp_port>\n", argv[0]);
        exit(1);
    }

    int tcp_port = atoi(argv[1]);
    int udp_port = atoi(argv[2]);

    int tcp_fd, udp_fd, fdmax;
    struct sockaddr_in tcp_addr, udp_addr, client_addr;
    fd_set master_set, read_fds;
    Inventory inv = {0};
    char buffer[BUF_SIZE];

    // TCP socket setup
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) { perror("tcp socket"); exit(1); }

    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(tcp_port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcp_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("tcp bind"); exit(1);
    }

    listen(tcp_fd, SOMAXCONN);

    // UDP socket setup
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) { perror("udp socket"); exit(1); }

    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(udp_port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("udp bind"); exit(1);
    }

    FD_ZERO(&master_set);
    FD_SET(tcp_fd, &master_set);
    FD_SET(udp_fd, &master_set);
    fdmax = tcp_fd > udp_fd ? tcp_fd : udp_fd;

    printf("molecule_supplier running (TCP: %d, UDP: %d)\n", tcp_port, udp_port);

    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select"); exit(1);
        }

        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == tcp_fd) {
                    // Accept TCP connection
                    socklen_t addrlen = sizeof(client_addr);
                    int client_fd = accept(tcp_fd, (struct sockaddr*)&client_addr, &addrlen);
                    if (client_fd >= 0) {
                        FD_SET(client_fd, &master_set);
                        if (client_fd > fdmax) fdmax = client_fd;
                        printf("atom_supplier connected\n");
                    }
                } else if (i == udp_fd) {
                    // Handle UDP request
                    socklen_t addrlen = sizeof(client_addr);
                    ssize_t len = recvfrom(udp_fd, buffer, BUF_SIZE - 1, 0,
                                           (struct sockaddr*)&client_addr, &addrlen);
                    if (len > 0) {
                        buffer[len] = 0;
                        char cmd[16], mol[32];
                        unsigned long long n;
                        if (sscanf(buffer, "%15s %31s %llu", cmd, mol, &n) == 3 && strcmp(cmd, "DELIVER") == 0) {
                            if (can_deliver(mol, n, &inv)) {
                                do_delivery(mol, n, &inv);
                                sendto(udp_fd, "DELIVERED\n", 10, 0,
                                       (struct sockaddr*)&client_addr, addrlen);
                            } else {
                                sendto(udp_fd, "FAILURE\n", 8, 0,
                                       (struct sockaddr*)&client_addr, addrlen);
                            }
                        } else {
                            sendto(udp_fd, "INVALID REQUEST\n", 17, 0,
                                   (struct sockaddr*)&client_addr, addrlen);
                        }
                    }
                } else {
                    // Handle TCP atom_supplier command
                    ssize_t len = recv(i, buffer, BUF_SIZE - 1, 0);
                    if (len <= 0) {
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    buffer[len] = 0;
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

    close(tcp_fd);
    close(udp_fd);
    return 0;
}
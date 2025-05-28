#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <errno.h>

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
    if (strcmp(molecule, "WATER") == 0)
        return inv->hydrogen >= 2 * count && inv->oxygen >= count;
    if (strcmp(molecule, "CARBON_DIOXIDE") == 0)
        return inv->carbon >= count && inv->oxygen >= 2 * count;
    if (strcmp(molecule, "GLUCOSE") == 0)
        return inv->carbon >= 6 * count && inv->hydrogen >= 12 * count && inv->oxygen >= 6 * count;
    if (strcmp(molecule, "ALCOHOL") == 0)
        return inv->carbon >= 2 * count && inv->hydrogen >= 6 * count && inv->oxygen >= count;
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

unsigned long long min(unsigned long long a, unsigned long long b) {
    return a < b ? a : b;
}

void handle_keyboard(Inventory* inv, const char* line) {
    if (strncmp(line, "GEN ", 4) != 0) return;
    line += 4;
    unsigned long long result = 0;

    if (strcmp(line, "SOFT DRINK") == 0) {
        result = inv->carbon / 7;
        result = min(result, inv->hydrogen / 14);
        result = min(result, inv->oxygen / 9);
    } else if (strcmp(line, "VODKA") == 0) {
        result = inv->carbon / 8;
        result = min(result, inv->hydrogen / 20);
        result = min(result, inv->oxygen / 8);
    } else if (strcmp(line, "CHAMPAGNE") == 0) {
        result = inv->carbon / 3;
        result = min(result, inv->hydrogen / 8);
        result = min(result, inv->oxygen / 4);
    }

    printf("can produce %llu of %s\n", result, line);
}

int main(int argc, char* argv[]) {
    int tcp_port = -1, udp_port = -1;
    int timeout = -1;
    Inventory inv = {0};

    struct option long_opts[] = {
        {"oxygen", required_argument, NULL, 'o'},
        {"carbon", required_argument, NULL, 'c'},
        {"hydrogen", required_argument, NULL, 'h'},
        {"timeout", required_argument, NULL, 't'},
        {"tcp-port", required_argument, NULL, 'T'},
        {"udp-port", required_argument, NULL, 'U'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:c:h:t:T:U:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'o': inv.oxygen = strtoull(optarg, NULL, 10); break;
            case 'c': inv.carbon = strtoull(optarg, NULL, 10); break;
            case 'h': inv.hydrogen = strtoull(optarg, NULL, 10); break;
            case 't': timeout = atoi(optarg); break;
            case 'T': tcp_port = atoi(optarg); break;
            case 'U': udp_port = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -T <tcp_port> -U <udp_port> [--oxygen N] [--carbon N] [--hydrogen N] [--timeout N]\n", argv[0]);
                exit(1);
        }
    }

    if (tcp_port <= 0 || udp_port <= 0) {
        fprintf(stderr, "Error: both --tcp-port and --udp-port must be specified.\n");
        exit(1);
    }

    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tcp_fd < 0 || udp_fd < 0) { perror("socket"); exit(1); }

    int optval = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in tcp_addr = {.sin_family = AF_INET, .sin_port = htons(tcp_port), .sin_addr.s_addr = INADDR_ANY};
    struct sockaddr_in udp_addr = {.sin_family = AF_INET, .sin_port = htons(udp_port), .sin_addr.s_addr = INADDR_ANY};

    bind(tcp_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr));
    listen(tcp_fd, SOMAXCONN);
    bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr));

    printf("drinks_bar running (TCP: %d, UDP: %d)\n", tcp_port, udp_port);

    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(tcp_fd, &master_set);
    FD_SET(udp_fd, &master_set);
    FD_SET(STDIN_FILENO, &master_set);
    int fdmax = tcp_fd > udp_fd ? tcp_fd : udp_fd;

    char buffer[BUF_SIZE];

    while (1) {
        read_fds = master_set;
        struct timeval tv = {0};
        struct timeval* ptv = NULL;
        if (timeout > 0) {
            tv.tv_sec = timeout;
            ptv = &tv;
        }

        int activity = select(fdmax + 1, &read_fds, NULL, NULL, ptv);
        if (activity < 0) { perror("select"); exit(1); }
        if (activity == 0) {
            printf("Timeout reached. Shutting down.\n");
            break;
        }

        for (int i = 0; i <= fdmax; i++) {
            if (!FD_ISSET(i, &read_fds)) continue;

            if (i == tcp_fd) {
                socklen_t len = sizeof(tcp_addr);
                int client_fd = accept(tcp_fd, (struct sockaddr*)&tcp_addr, &len);
                if (client_fd >= 0) {
                    FD_SET(client_fd, &master_set);
                    if (client_fd > fdmax) fdmax = client_fd;
                    printf("atom_supplier connected\n");
                }
            } else if (i == udp_fd) {
                socklen_t len = sizeof(udp_addr);
                ssize_t n = recvfrom(udp_fd, buffer, BUF_SIZE - 1, 0, (struct sockaddr*)&udp_addr, &len);
                if (n > 0) {
                    buffer[n] = 0;
                    char cmd[16], mol[32];
                    unsigned long long count;
                    if (sscanf(buffer, "%15s %31s %llu", cmd, mol, &count) == 3 && strcmp(cmd, "DELIVER") == 0) {
                        if (can_deliver(mol, count, &inv)) {
                            do_delivery(mol, count, &inv);
                            sendto(udp_fd, "DELIVERED\n", 10, 0, (struct sockaddr*)&udp_addr, len);
                        } else {
                            sendto(udp_fd, "FAILURE\n", 8, 0, (struct sockaddr*)&udp_addr, len);
                        }
                    } else {
                        sendto(udp_fd, "INVALID REQUEST\n", 17, 0, (struct sockaddr*)&udp_addr, len);
                    }
                }
            } else if (i == STDIN_FILENO) {
                if (fgets(buffer, sizeof(buffer), stdin)) {
                    buffer[strcspn(buffer, "\n")] = 0;
                    handle_keyboard(&inv, buffer);
                }
            } else {
                ssize_t len = recv(i, buffer, BUF_SIZE - 1, 0);
                if (len <= 0) {
                    close(i);
                    FD_CLR(i, &master_set);
                } else {
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 5555
#define BUF_SIZE 1024

typedef struct {
    int carbon;
    int hydrogen;
    int oxygen;
} Inventory;

void update_inventory(Inventory* inv, const char* atom, int amount) {
    if (strcmp(atom, "CARBON") == 0) inv->carbon += amount;
    else if (strcmp(atom, "HYDROGEN") == 0) inv->hydrogen += amount;
    else if (strcmp(atom, "OXYGEN") == 0) inv->oxygen += amount;
}

void print_inventory(int client_fd, Inventory* inv) {
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg),
             "Inventory: CARBON=%d, HYDROGEN=%d, OXYGEN=%d\n",
             inv->carbon, inv->hydrogen, inv->oxygen);
    send(client_fd, msg, strlen(msg), 0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    char buffer[BUF_SIZE];
    Inventory inv = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(1);
    }

    listen(server_fd, 1);
    printf("Atom warehouse server listening on port %d...\n", PORT);

    socklen_t addrlen = sizeof(address);
    client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (client_fd < 0) {
        perror("accept");
        exit(1);
    }

    while (1) {
        memset(buffer, 0, BUF_SIZE);
        ssize_t len = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        if (len <= 0) break;

        char cmd[16], atom[16];
        int amount;

        if (sscanf(buffer, "%s %s %d", cmd, atom, &amount) == 3 && strcmp(cmd, "ADD") == 0) {
            update_inventory(&inv, atom, amount);
            print_inventory(client_fd, &inv);
        } else {
            char* err = "Invalid command. Use: ADD ATOM_NAME AMOUNT\n";
            send(client_fd, err, strlen(err), 0);
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
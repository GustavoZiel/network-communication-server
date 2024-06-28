#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct client {
    int id;
    char name[20];
    int socket;
    bool active;
    pthread_t thread;
} client_t;

client_t *clients = NULL;
int qtt_clients = 0;

int create_socket() {
    int server_fd;
    // Criando um servidor IPv4 (AF_INET) com conexão TCP (SOCK_STREAM)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[Error] Socket could not be created");
        exit(EXIT_FAILURE);
    }
    printf("\n => Socket created with success\n");
    return server_fd;
}

void config_socket(int server_fd) {
    int opt = 1;
    // SO_REUSEADDR : bind to address already in use (in case is needed to restart)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("[Error] Setting socket options failed");
        exit(EXIT_FAILURE);
    }
    printf(" => Socket options set with success\n");
}

void set_address(struct sockaddr_in *address, int port) {
    (*address).sin_family = AF_INET;
    (*address).sin_addr.s_addr = INADDR_ANY;
    (*address).sin_port = htons(port);
}

void start_socket(int server_fd, struct sockaddr_in address) {
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Error] Socket could not be binded");
        exit(EXIT_FAILURE);
    }
    printf(" => Socket binded to all interfaces and to port [%d]\n", ntohs(address.sin_port));

    int queue_size = 3;
    if (listen(server_fd, queue_size) < 0) {
        perror("[Error] Queueing failed");
        exit(EXIT_FAILURE);
    }
    printf(" => Socket is listening...\n");
}

void broadcast_msg(char *msg, int id) {
    for (int i = 0; i < qtt_clients; i++) {
        if (i != id && clients[i].active == true) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
}

void *send_n_read(int id) {
    char buffer[512];
    char msg[1024];

    while (memset(buffer, 0, sizeof(buffer)) && read(clients[id].socket, buffer, 500) != 0) {
        if (!strcmp("/q", buffer)) {
            snprintf(msg, sizeof(msg), "[%s] left chat.\n", clients[id].name);
            clients[id].active = false;
            broadcast_msg(msg, clients[id].id);
            printf("%s", msg);
            break;
        } else {
            snprintf(msg, sizeof(msg), "[%s]: %s\n", clients[id].name, buffer);
            broadcast_msg(msg, clients[id].id);
            printf("%s", msg);
        }
    }
    close(clients[id].socket);
    return NULL;
}

void register_client(int id) {
    char *welcome_msg = "\n===== WELCOME TO THE CHAT GROUP =====\n";
    send(clients[id].socket, welcome_msg, strlen(welcome_msg), 0);

    char *register_msg = "What is your name ?\n";
    send(clients[id].socket, register_msg, strlen(register_msg), 0);

    // Getting client name
    char name[20] = {0};
    read(clients[id].socket, name, 19);
    strcpy(clients[id].name, name);

    char *end_msg = "=====================================\n\n";
    send(clients[id].socket, end_msg, strlen(end_msg), 0);

    char *chat_start = "Chat:\n\n";
    send(clients[id].socket, chat_start, strlen(chat_start), 0);

    clients[id].active = true;
}

void get_info(int client) {
    struct sockaddr_in client_addr;

    socklen_t client_addr_len = sizeof(client_addr);
    getpeername(client, (struct sockaddr *)&client_addr, &client_addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    printf("\n======== Connection %d =========\n", qtt_clients + 1);
    printf("Client IP address: %s\n", client_ip);
    printf("Client port: %d\n", ntohs(client_addr.sin_port));
    printf("===============================\n\n");
}

void add_client(int client_socket) {
    clients = (client_t *)realloc(clients, (++qtt_clients)*sizeof(client_t));
    if (clients == NULL) {
        perror("Failed to allocate memory for clients");
        free(clients);
        exit(1);
    }
    clients[qtt_clients-1].id = qtt_clients-1;
    clients[qtt_clients-1].socket = client_socket;
    clients[qtt_clients-1].active = false;
}

void *handle_client(void *args) {
    int *id = (int *)args;
    int id_client = *id;
    register_client(id_client);
    send_n_read(id_client);
    return NULL;
}

void handle_server(int server_fd, struct sockaddr_in address, int await_time) {
    fd_set readfds;
    int client_socket, id_client;
    socklen_t addrlen = sizeof(address);

    // Creating timeout object
    struct timeval *timeout = NULL;
    if (await_time > 0) {
        timeout = malloc(sizeof(struct timeval));
        timeout->tv_sec = await_time;
        timeout->tv_usec = 0;
        printf("\n==== The socket will wait %ds for connections ====\n", await_time);

    } else {
        printf("\n==== The socket is wainting connections ====\n");
    }

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // Wait for activity on any of the sockets with timeout
        int activity = select(max_sd + 1, &readfds, NULL, NULL, timeout);

        if (activity == -1) {
            perror("[Error] Wainting for sockets activity failed");
            exit(EXIT_FAILURE);
        } else if (activity == 0) {
            // Timeout occurred, exit loop
            break;
        }

        // Check if server socket has activity
        if (FD_ISSET(server_fd, &readfds)) {
            // Accept a new connection
            if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("[Error] Accepting new connection failed");
                exit(EXIT_FAILURE);
            }

            get_info(client_socket);
            add_client(client_socket);
            id_client = qtt_clients - 1;

            // Create thread to handle new connection
            if (pthread_create(&clients[qtt_clients - 1].thread, NULL, handle_client, (void *)&id_client) != 0) {
                perror("[Error] Creation of a thread for a client failed");
                close(client_socket);
            }
        }
    }
    free(timeout);

    printf("==== The socket is closed for futher connections ====\n");

    // Wait for each thread to finish
    for (int i = 0; i < qtt_clients; i++) {
        if (pthread_join(clients[i].thread, NULL) != 0) {
            perror("[Error] Waiting for threads to finish");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <PORT> <TIMEOUT>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int timeout = atoi(argv[2]);

    int server_fd;
    server_fd = create_socket();
    config_socket(server_fd);

    struct sockaddr_in address;
    set_address(&address, port);
    start_socket(server_fd, address);

    printf("\n=================================\n");
    printf("\tThe socket is on\t\n");
    printf("=================================\n");

    handle_server(server_fd, address, timeout);

    printf("\n=================================\n");
    printf("\tThe socket is off\t\n");
    printf("=================================\n");

    free(clients);
    // closing the listening socket
    close(server_fd);

    return 0;
}

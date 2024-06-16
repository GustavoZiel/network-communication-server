#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define AWAIT_TIME 5

typedef struct client {
    int id;
    char name[20];
    int socket;
    pthread_t thread;
} client_t;

client_t clients[MAX_CLIENTS];
int qtt_clients = 0;

void broadcast_msg(char *msg, int id) {
    for (int i = 0; i < qtt_clients; i++) {
        if (i != id) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
}

void *send_n_read(void *args) {
    client_t client = *(client_t *)args;
    char buffer[512];
    char msg[1024];
    while (memset(buffer, 0, sizeof(buffer)) &&
           read(client.socket, buffer, 500) != 0) {
        if (!strcmp("/q", buffer)) {
            snprintf(msg, sizeof(msg), "[%s] saiu do chat.", client.name);
        } else {
            snprintf(msg, sizeof(msg), "[%s]: %s", client.name, buffer);
        }
        printf("%s\n", msg);
        broadcast_msg(msg, client.id);
    }
    close(client.socket);
    return NULL;
}

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

void set_address(struct sockaddr_in *address) {
    (*address).sin_family = AF_INET;
    (*address).sin_addr.s_addr = INADDR_ANY;
    (*address).sin_port = htons(PORT);
}

void start_socket(int server_fd, struct sockaddr_in address) {
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Error] Socket could not be binded");
        exit(EXIT_FAILURE);
    }
    printf(" => Socket binded to ip [%d] and to port [%d]\n", INADDR_ANY, PORT);

    int queue_size = 3;
    if (listen(server_fd, queue_size) < 0) {
        perror("[Error] Queueing failed");
        exit(EXIT_FAILURE);
    }
    printf(" => Socket is listening...\n");
}

void add_client(int client_socket) {
    qtt_clients += 1;
    clients[qtt_clients - 1].id = qtt_clients - 1;
    clients[qtt_clients - 1].socket = client_socket;
}

void register_client(client_t *client) {
    char *welcome_msg = "\n===== WELCOME TO THE CHAT GROUP =====\n";
    send(client->socket, welcome_msg, strlen(welcome_msg), 0);

    char *register_msg = "What is your name ?";
    send(client->socket, register_msg, strlen(register_msg), 0);

    // TODO : testar tamanho dos nomes
    char name[20] = {0};
    int size;
    size = read(client->socket, name, 19);
    strcpy(client->name, name);

    char *end_msg = "=====================================\n";
    send(client->socket, end_msg, strlen(end_msg), 0);
}

void *handle_client(void *args) {
    client_t *client = (client_t *)args;
    register_client(client);
    send_n_read(client);
    return NULL;
}

void get_info(int client) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    getpeername(client, (struct sockaddr *)&client_addr, &client_addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    printf("======== Connection %d =========\n", qtt_clients + 1);
    printf("Client IP address: %s\n", client_ip);
    printf("Client port: %d\n", ntohs(client_addr.sin_port));
    printf("===============================\n");
}

void handle_server(int server_fd, struct sockaddr_in address) {
    fd_set readfds;
    int client_socket;
    socklen_t addrlen = sizeof(address);

    // Wait for activity on any of the sockets with timeout
    struct timeval *timeout = NULL;
    if (AWAIT_TIME != 0) {
        timeout = malloc(sizeof(struct timeval));
        timeout->tv_sec = AWAIT_TIME;
        timeout->tv_usec = 0;
        printf("\n==== The socket will wait %ds for connections ====\n", AWAIT_TIME);

    } else {
        printf("\n==== The socket is wainting connections ====\n");
    }

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // To use the timeout
        int activity = select(max_sd + 1, &readfds, NULL, NULL, timeout);

        // Not use timeout (server runs unstopping)
        // int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity == -1) {
            perror("[Error] Wainting for sockets activity failed");
            exit(EXIT_FAILURE);
        } else if (activity == 0) {
            // TODO : Testar isso aqui, roda até ficar inativo por time out ?
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

            // Create thread to handle new connection
            if (pthread_create(&clients[qtt_clients - 1].thread, NULL, handle_client, (void *)&clients[qtt_clients - 1]) != 0) {
                perror("[Error] Creation of a thread for a client failed");
                close(client_socket);
            }
        }
    }
    free(timeout);

    printf("\n==== The socket is closed for futher connections ====\n");

    // Wait for each thread to finish
    for (int i = 0; i < qtt_clients; i++) {
        if (pthread_join(clients[i].thread, NULL) != 0) {
            perror("[Error] Waiting thread to finish failed");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char const *argv[]) {
    int server_fd;
    server_fd = create_socket();

    config_socket(server_fd);

    struct sockaddr_in address;
    set_address(&address);
    start_socket(server_fd, address);

    handle_server(server_fd, address);

    printf("\n=================================\n");
    printf("\tThe socket is off\t\n");
    printf("=================================\n");

    // closing the listening socket
    close(server_fd);

    return 0;
}

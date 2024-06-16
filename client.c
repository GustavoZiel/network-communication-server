#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define IP_ADDRESS "192.168.15.7"
#define PORT 8080

// Variável de controle para indicar se a thread deve continuar executando
int running = 1;

// Mutex para garantir acesso seguro à variável running
pthread_mutex_t mutex_running = PTHREAD_MUTEX_INITIALIZER;

int create_socket() {
    int client_fd;
    // Criando um servidor IPv4 (AF_INET) com conexão TCP (SOCK_STREAM)
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(" [Error] Socket could not be created\n");
        exit(EXIT_FAILURE);
    }
    printf("\n => Socket created with success\n");

    return client_fd;
}

void config_socket(struct sockaddr_in* serv_addr) {
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP_ADDRESS, &serv_addr->sin_addr) <= 0) {
        printf("\n [Error] Invalid address / Address not supported \n");
        exit(EXIT_FAILURE);
    }

    printf(" => Socket options set with success\n");
}

void client_connect(int client_fd, struct sockaddr_in serv_addr) {
    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n [Error] Connection Failed \n");
        exit(EXIT_FAILURE);
    }
    printf(" => Connection established with success\n");
}

void readline(char** str) {
    if (*str != NULL) {
        free(*str);
        *str = NULL;
    }
    char c;
    int size = 0;
    while ((c = getchar()) != '\n' && c != EOF) {
        (*str) = realloc((*str), (++size) * sizeof(char*));
        ((*str))[size - 1] = c;
    }
    (*str) = realloc((*str), (size + 1) * sizeof(char*));
    (*str)[size] = '\0';
}

void* write_server(void* args) {
    int client_fd = *(int*)args;

    char* buffer = NULL;
    do {
        // Lê o que o cliente digita
        readline(&buffer);
        // Envia para o servidor
        send(client_fd, buffer, strlen(buffer), 0);
        // Até o cliente digitar o comando de saída (/q)
    } while (strcmp(buffer, "/q"));
    free(buffer);

    pthread_mutex_lock(&mutex_running);
    running = 0;
    pthread_mutex_unlock(&mutex_running);
    return NULL;
}

void* read_server(void* args) {
    int client_fd = *(int*)args;
    // Tamanho da linha que ele recebe do servidor
    char buffer[1024];

    while (1) {
        pthread_mutex_lock(&mutex_running);
        if (running == 0) {
            break;
        }
        pthread_mutex_unlock(&mutex_running);

        // Cliente recebe mensagens do servidor
        memset(buffer, 0, sizeof(buffer));
        if (read(client_fd, buffer, 1024) > 0) {
            printf("%s\n", buffer);
        }
    }

    return NULL;
}

int main(int argc, char const* argv[]) {
    int status, client_fd;
    struct sockaddr_in serv_addr;

    client_fd = create_socket();
    config_socket(&serv_addr);

    client_connect(client_fd, serv_addr);

    /*
    Setando o file_descriptor do client (client_fd) como non-blocking, não bloqueara a
    execução do programa em operações que causam blocking (read, write). Necessário, visto
    que o servidor pode ter vários clientes sendo antendidos ao mesmo tempo
    */
    // fcntl(client_fd, F_SETFL, O_NONBLOCK);

    pthread_t read_t, write_t;
    pthread_create(&write_t, NULL, write_server, &client_fd);
    pthread_create(&read_t, NULL, read_server, &client_fd);

    pthread_join(write_t, NULL);
    pthread_join(read_t, NULL);

    close(client_fd);

    return 0;
}

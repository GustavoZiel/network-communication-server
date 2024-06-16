// TODO ip -f inet addr show 

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

void* ler(void* args) {
    int client_fd = *(int*)args;
    char buffer[1024];
    while (1) {
        pthread_mutex_lock(&mutex_running);
        if (running == 0) {
            break;
        }
        pthread_mutex_unlock(&mutex_running);
        memset(buffer, 0, sizeof(buffer));
        if (read(client_fd, buffer, 1024 - 1) > 0) {
            printf("%s\n", buffer);
        }
    }
    return NULL;
}

void* escrever(void* args) {
    int client_fd = *(int*)args;
    char* buffer = NULL;
    do {
        readline(&buffer);
        send(client_fd, buffer, strlen(buffer), 0);
    } while (strcmp(buffer, "/q"));
    free(buffer);
    pthread_mutex_lock(&mutex_running);
    running = 0;
    pthread_mutex_unlock(&mutex_running);
    return NULL;
}

int main(int argc, char const* argv[]) {
    printf("\n");
    
    int status, client_fd;
    struct sockaddr_in serv_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(" [Error] Socket could not be created\n");
        return EXIT_FAILURE;
    }
    printf(" => Socket created with success\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_ADDRESS, &serv_addr.sin_addr) <= 0) {
        printf("\n [Error] Invalid address / Address not supported \n");
        return EXIT_FAILURE;
    }

    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\n [Error] Connection Failed \n");
        return EXIT_FAILURE;
    }
    printf(" => Connection establish with success\n");

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    pthread_t ler_t, escrever_t;
    pthread_create(&escrever_t, NULL, escrever, &client_fd);
    pthread_create(&ler_t, NULL, ler, &client_fd);

    pthread_join(escrever_t, NULL);
    pthread_join(ler_t, NULL);

    close(client_fd);

    return 0;
}

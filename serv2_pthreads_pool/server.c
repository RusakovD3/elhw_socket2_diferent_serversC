#include <stdio.h>      // Стандартные функции ввода-вывода.
#include <stdlib.h>     // Функции для управления памятью, контроля процесса.
#include <string.h>     // Функции для работы со строками.
#include <unistd.h>     // Для различных констант и типов и функции sleep.
#include <pthread.h>    // Для работы с потоками.
#include <sys/socket.h> // Для работы с сокетами.
#include <netinet/in.h> // Для структур данных IP.
#include <time.h>       // Для работы с временем.


// Определение структуры для информации о потоке
#define THREAD_COUNT 4
#define BASE_PORT 8880

typedef struct {
    int port;
    int available;
} thread_info;

thread_info thread_pool[THREAD_COUNT];

// Функция которая запускается в потоке
void* time_server(void* arg) {
    thread_info* info = (thread_info*)arg;
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[256];
    time_t ticks;

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // создаёт сокет
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(info->port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { // привязывает сокет к адресу
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5); // начинает слушать сокет
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); // принимает входящие подключения
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        ticks = time(NULL);
        snprintf(buffer, sizeof(buffer), "Time: %.24s\r\n\n", ctime(&ticks));
        write(newsockfd, buffer, strlen(buffer)); // отправляет время
        shutdown(newsockfd, SHUT_WR);
        close(newsockfd);
        sleep(2);
        info->available = 1;
    }
    close(sockfd);
    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int i;

    for (i = 0; i < THREAD_COUNT; ++i) {
        thread_pool[i].port = BASE_PORT + i;
        thread_pool[i].available = 1;
        pthread_create(&threads[i], NULL, time_server, &thread_pool[i]);
    }

    int master_sockfd, newsockfd, cli_port;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    master_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (master_sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);

    if (bind(master_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(master_sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(master_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        for (i = 0; i < THREAD_COUNT; ++i) {
            if (thread_pool[i].available) {
                cli_port = thread_pool[i].port;
                thread_pool[i].available = 0;
                break;
            }
        }

        char msg[20];
        sprintf(msg, "Connect to port %d", cli_port);
        write(newsockfd, msg, strlen(msg));
        close(newsockfd);
    }
    close(master_sockfd);
    return 0;
}

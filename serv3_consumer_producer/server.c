#include <stdio.h>      // Библиотека стандартного ввода-вывода.
#include <stdlib.h>     // Библиотека стандартных функций, включая управление памятью и процессами.
#include <unistd.h>     // Библиотека для доступа к POSIX API, включая системные вызовы.
#include <string.h>     // Библиотека функций для работы со строками.
#include <pthread.h>    // Библиотека для работы с потоками.
#include <sys/socket.h> // Библиотека сокетов.
#include <netinet/in.h> // Библиотека, определяющая структуры для сетевых операций.
#include <time.h>       // Библиотека для работы с временем.

#define THREAD_POOL_SIZE 4  // Размер пула потоков.
#define QUEUE_SIZE 5        // Размер очереди клиентских сокетов.

int client_queue[QUEUE_SIZE];  // Очередь для хранения клиентских сокетов.
int queue_count = 0;           // Счетчик элементов в очереди.

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для синхронизации доступа к очереди.
pthread_cond_t cond_non_full = PTHREAD_COND_INITIALIZER;  // Условная переменная, сигнализирующая, что очередь не полна.
pthread_cond_t cond_non_empty = PTHREAD_COND_INITIALIZER; // Условная переменная, сигнализирующая, что очередь не пуста.

// Функция выполняемая потоками
void* handle_client() {
    while (1) {
        pthread_mutex_lock(&mutex); // Блокируем мьютекс для защиты доступа к очереди.

        while (queue_count == 0) { // Проверяем, не пуста ли очередь.
            pthread_cond_wait(&cond_non_empty, &mutex); // Ожидаем появления элементов в очереди.
        }

        // Извлекаем сокет из очереди.
        int client_socket = client_queue[0];
        for (int i = 0; i < queue_count - 1; i++) {
            client_queue[i] = client_queue[i + 1];
        }
        queue_count--;
        
        pthread_cond_signal(&cond_non_full); // Сигнализируем, что в очереди появилось свободное место.
        pthread_mutex_unlock(&mutex); // Разблокируем мьютекс.

        // Отправляем клиенту текущее время.
        char buffer[256];
        time_t ticks = time(NULL);
        snprintf(buffer, sizeof(buffer), "%.24s\r\n", ctime(&ticks));
        write(client_socket, buffer, strlen(buffer));
        close(client_socket);
    }
}

int main() {
    int sockfd, newsockfd, portno = 7777;  // Сокеты и номер порта.
    struct sockaddr_in serv_addr, cli_addr;  // Структуры адресов сервера и клиента.
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Создание TCP сокета.
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);  // Начинаем прослушивание сокета.
    clilen = sizeof(cli_addr);

    // Создание потоков обработчиков.
    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threads[i], NULL, handle_client, NULL);
    }

    // Главный цикл для принятия подключений.
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        pthread_mutex_lock(&mutex);
        while (queue_count == QUEUE_SIZE) { // Проверяем, полна ли очередь.
            pthread_cond_wait(&cond_non_full, &mutex); // Ожидаем освобождения места.
        }

        client_queue[queue_count++] = newsockfd; // Добавляем новое подключение в очередь.
        pthread_cond_signal(&cond_non_empty); // Сигнализируем о непустой очереди.
        pthread_mutex_unlock(&mutex);
    }

    close(sockfd);  // Закрываем основной сокет.
    return 0;
}

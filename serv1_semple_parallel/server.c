#include <stdio.h>       // Базовый ввод-вывод
#include <stdlib.h>      // Стандартная библиотека для malloc, exit и др.
#include <string.h>      // Обработка строк
#include <unistd.h>      // Различные системные вызовы UNIX
#include <time.h>        // Для получения и форматирования времени
#include <pthread.h>     // Для работы с потоками
#include <sys/types.h>   // Определения типов данных
#include <sys/socket.h>  // Сетевое программирование
#include <netinet/in.h>  // Структуры для работы с адресами


void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc; // Получаем дескриптор сокета из аргумента
    char buffer[256];              // Буфер для отправки данных
    time_t ticks;                  // Переменная для текущего времени
    
    ticks = time(NULL);            // Получаем текущее время
    snprintf(buffer, sizeof(buffer), "%.24s\r\n", ctime(&ticks)); // Форматируем время в строку
    write(sock, buffer, strlen(buffer)); // Отправляем строку клиенту
    
    close(sock);                   // Закрываем сокет
    free(socket_desc);             // Освобождаем память, выделенную для дескриптора сокета
    
    return NULL;                   // Возвращаем NULL, т.к. функция должна возвращать void*
}

int main() {
    int sockfd, newsockfd, *new_sock;        // Дескрипторы сокетов и указатель для потока
    socklen_t clilen;                        // Размер структуры с адресом клиента
    struct sockaddr_in serv_addr, cli_addr;  // Структуры адресов сервера и клиента
    pthread_t thread_id;                     // Идентификатор потока

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Создаем TCP сокет
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));  // Обнуляем структуру
    serv_addr.sin_family = AF_INET;            // Устанавливаем тип адресов IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;    // Принимаем подключения на все адреса сервера
    serv_addr.sin_port = htons(8888);          // Порт сервера (8888)

    // bind - привязывает сокет к адресу, который содержится в структуре serv_addr
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);                         // Начинаем слушать сокет, макс. очередь 5
    clilen = sizeof(cli_addr);                 // Размер структуры адреса клиента

    while (1) {                                // Бесконечный цикл для приема подключений
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);  // Принимаем новое подключение
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        new_sock = malloc(1);                   // Выделяем память под дескриптор сокета
        *new_sock = newsockfd;
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) < 0) {
            perror("ERROR creating thread");
            exit(1);
        }
    }
    
    close(sockfd);  // Закрываем слушающий сокет (теоретически никогда не достигнет)
    return 0;
}

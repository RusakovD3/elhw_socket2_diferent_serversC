#include <stdio.h>    // Базовый ввод-вывод
#include <stdlib.h>   // Для exit
#include <unistd.h>   // Для close
#include <string.h>   // Для memset
#include <sys/types.h>  // Типы данных
#include <sys/socket.h> // Сокеты
#include <netinet/in.h> // Структуры адресов
#include <netdb.h>      // Для работы с DNS

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;                       // Дескриптор сокета, номер порта и переменная для кол-ва прочитанных байт
    struct sockaddr_in serv_addr;                // Структура адреса сервера
    struct hostent *server;                      // Для информации о сервере

    char buffer[256];                            // Буфер для получения данных
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);                      // Парсим номер порта из аргументов

    sockfd = socket(AF_INET, SOCK_STREAM, 0);    // Создаем TCP сокет
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);             // Получаем информацию о сервере по имени
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));    // Обнуляем структуру
    serv_addr.sin_family = AF_INET;              // Тип адресов IPv4
    memcpy(&server->h_addr, server->h_addr_list[0], server->h_length);  // Копируем адрес сервера
    serv_addr.sin_port = htons(portno);          // Устанавливаем порт

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)  // Подключаемся к серверу
        error("ERROR connecting");

    memset(buffer, 0, 256);                       // Обнуляем буфер
    n = read(sockfd, buffer, 255);                // Читаем данные с сервера
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);                        // Выводим полученные данные

    close(sockfd);                                // Закрываем сокет
    return 0;
}

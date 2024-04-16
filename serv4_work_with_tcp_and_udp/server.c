#include <stdio.h>       // Библиотека для стандартного ввода/вывода.
#include <stdlib.h>      // Библиотека для функций управления памятью и процессами, таких как exit.
#include <string.h>      // Библиотека для работы со строками.
#include <unistd.h>      // Библиотека для доступа к POSIX API.
#include <sys/socket.h>  // Библиотека для работы с сокетами.
#include <netinet/in.h>  // Библиотека для интернет-протоколов.
#include <sys/epoll.h>   // Библиотека для работы с epoll.
#include <time.h>        // Библиотека для работы с временем.

#define MAX_EVENTS 5   // Максимальное количество событий, которые epoll может обрабатывать одновременно.
#define PORT 8888      // Порт, который будет слушать сервер.

void error(const char *msg) {
    perror(msg);       // Выводит сообщение об ошибке в stderr.
    exit(1);           // Выходит из программы с кодом ошибки 1.
}

int main() {
    int tcp_sock, udp_sock, port = PORT;     // Дескрипторы сокетов и номер порта.
    struct sockaddr_in addr;                 // Структура, описывающая IP адрес.
    int epoll_fd, n;                         // Дескриптор epoll и переменная для количества событий.
    struct epoll_event event, events[MAX_EVENTS];  // Структура для событий epoll.

    // Создаем TCP сокет.
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) error("ERROR opening TCP socket");

    // Создаем UDP сокет.
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) error("ERROR opening UDP socket");

    memset(&addr, 0, sizeof(addr));          // Очищаем структуру адреса.
    addr.sin_family = AF_INET;               // Задаем семейство адресов IPv4.
    addr.sin_addr.s_addr = INADDR_ANY;       // Принимаем подключения на любом адресе сервера.
    addr.sin_port = htons(port);             // Задаем порт, преобразуя его из хостового в сетевой порядок байтов.

    // Привязываем TCP сокет.
    if (bind(tcp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        error("ERROR on binding TCP socket");

    // Привязываем UDP сокет.
    if (bind(udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        error("ERROR on binding UDP socket");

    listen(tcp_sock, 5);  // Начинаем прослушивание TCP сокета.

    // Создаем epoll инстанс.
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) error("ERROR creating epoll instance");

    // Добавляем TCP сокет в epoll.
    event.data.fd = tcp_sock;
    event.events = EPOLLIN;  // Указываем, что нас интересует событие "готовность к чтению".
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tcp_sock, &event) < 0)
        error("ERROR adding TCP socket to epoll");

    // Добавляем UDP сокет в epoll.
    event.data.fd = udp_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_sock, &event) < 0)
        error("ERROR adding UDP socket to epoll");

    while (1) {
        n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);  // Ожидаем события в любом из сокетов.
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == tcp_sock) {
                // Обработка TCP подключения.
                int newsockfd = accept(tcp_sock, NULL, NULL);  // Принимаем новое подключение.
                if (newsockfd < 0) error("ERROR on accept");

                char buffer[256];
                time_t ticks = time(NULL);  // Получаем текущее время.
                snprintf(buffer, sizeof(buffer), "%.24s\r\n", ctime(&ticks));  // Форматируем время в строку.
                write(newsockfd, buffer, strlen(buffer));  // Отправляем время клиенту.
                close(newsockfd);  // Закрываем сокет клиента.
            } else if (events[i].data.fd == udp_sock) {
                // Обработка UDP сообщения.
                char buffer[256];
                struct sockaddr_in cli_addr;
                socklen_t clilen = sizeof(cli_addr);

                int len = recvfrom(udp_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);  // Получаем данные.
                if (len < 0) error("ERROR on recvfrom");

                time_t ticks = time(NULL);
                snprintf(buffer, sizeof(buffer), "%.24s\r\n", ctime(&ticks));  // Форматируем текущее время в строку.
                sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, clilen);  // Отправляем время обратно клиенту.
            }
        }
    }

    close(tcp_sock);  // Закрываем TCP сокет.
    close(udp_sock);  // Закрываем UDP сокет.
    close(epoll_fd);  // Закрываем epoll файл.
    
    return 0;
}

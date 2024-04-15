#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[256];
    time_t ticks;
    
    ticks = time(NULL);
    snprintf(buffer, sizeof(buffer), "%.24s\r\n", ctime(&ticks));
    write(sock, buffer, strlen(buffer));
    
    close(sock);
    free(socket_desc);
    
    return NULL;
}

int main() {
    int sockfd, newsockfd, *new_sock;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t thread_id;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        new_sock = malloc(1);
        *new_sock = newsockfd;
        if (pthread_create(&thread_id, NULL, client_handler, (void*) new_sock) < 0) {
            perror("ERROR creating thread");
            exit(1);
        }
    }
    
    close(sockfd);
    return 0;
}

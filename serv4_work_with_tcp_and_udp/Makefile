CC = gcc
CFLAGS = -Wall -Werror -Wextra

.PHONY: all clean

all: server tcp_client udp_client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

tcp_client: client_tcp.c
	$(CC) $(CFLAGS) -o tcp_client client_tcp.c

udp_client: client_udp.c
	$(CC) $(CFLAGS) -o udp_client client_udp.c

clean:
	rm -f server udp_client tcp_client
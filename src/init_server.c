#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "../include/send.h"
#include "../include/http_methods.h"

#define PORT 8080
#define MAX_CONNECTIONS 5

int setup_server() {
	int server_socket, client_socket;
	int epoll_fd, event_count;
	struct epoll_event event, events[MAX_CONNECTIONS];
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);


	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		printf("Socket creation failed %d %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Bind failed %d %s\n", errno, strerror(errno));
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, MAX_CONNECTIONS) < 0) {
		printf("Listen failed %d %s\n", errno, strerror(errno));
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	epoll_fd = epoll_create1(0);
	if (epoll_fd <= 0) {
		printf("Epoll create failed %d %s\n", errno, strerror(errno));
		close(server_socket);
	}

	event.events = EPOLLIN;
	event.data.fd = server_socket;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
		printf("Epoll ctl failed %d %s\n", errno, strerror(errno));
		close(server_socket);
		close(epoll_fd);
		exit(EXIT_FAILURE);
	}

	printf("Server listening on port %d\n", PORT);

	while (1) {
		event_count = epoll_wait(epoll_fd, events, MAX_CONNECTIONS, -1);
		for (int i = 0; i < event_count; i++) {
			if (events[i].data.fd == server_socket) {
				client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
				if (client_socket == -1) continue;

				event.events = EPOLLIN;
				event.data.fd = client_socket;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
				printf("New client connected %d\n", client_socket);
			} else {
				int client_fd = events[i].data.fd;
				char buffer[1024];
				ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
				if (bytes_read <= 0) {
					close(client_fd);
				} else {
					buffer[bytes_read] = '\0';
					char method[16], path[256], protocol[16];
					sscanf(buffer, "%s %s %s", method, path, protocol);
					printf("Received request: %s %s %s\n", method, path, protocol);
					if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
						send_html(client_fd, "file/index.html");
					} else if (strcmp(path, "/test-404") == 0) {
						http_get("https://httpbin.org/status/404", client_fd);
					} else if (strcmp(path, "/test-403") == 0) {
						http_get("https://httpbin.org/status/403", client_fd);
					} else if (strcmp(path, "/test-501") == 0) {
						http_get("https://httpbin.org/status/501", client_fd);
					} else if (strcmp(path, "/test-400") == 0) {
						http_get("https://httpbin.org/status/400", client_fd);
					} else if (strcmp(path, "/broken-link") == 0) {
						http_get("https://httpbin.org/status/503", client_fd);
					} else {
						send_error_html(client_fd, "file/404.html", 404);
					}
					close(client_fd);
				}
			}
		}
	}

	close(server_socket);
	return client_socket;
}
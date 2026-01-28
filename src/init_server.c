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
#include "../include/config.h"
#include "../include/logger.h"


int setup_server() {

	ServerConfig config;
	load_config("config.json", &config);

	LogLevel log_level = config.debug_mode ? LOG_DEBUG : LOG_INFO;
	logger_init(log_level, config.log_file);

	log_msg(LOG_INFO, "Server configuration loaded: IP=%s, Port=%d, Max Connections=%d, Root Dir=%s, Log File=%s",
		config.ip, config.port, config.max_connections, config.root_directory, config.log_file);
	
	log_msg(LOG_DEBUG, "Debug mode is ON. Detailed logs enabled.");

	int server_socket, client_socket;
	int epoll_fd, event_count;
	struct epoll_event event, events[config.max_connections];
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);


	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		log_msg(LOG_ERROR, "Socket creation failed %d %s", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(config.port);

	int optval = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		log_msg(LOG_ERROR, "Set socket options failed %d %s", errno, strerror(errno));
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		log_msg(LOG_ERROR, "Bind failed %d %s", errno, strerror(errno));
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, config.max_connections) < 0) {
		log_msg(LOG_ERROR, "Listen failed %d %s", errno, strerror(errno));
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	epoll_fd = epoll_create1(0);
	if (epoll_fd <= 0) {
		log_msg(LOG_ERROR, "Epoll create failed %d %s", errno, strerror(errno));
		close(server_socket);
	}

	event.events = EPOLLIN;
	event.data.fd = server_socket;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
		log_msg(LOG_ERROR, "Epoll ctl failed %d %s", errno, strerror(errno));
		close(server_socket);
		close(epoll_fd);
		exit(EXIT_FAILURE);
	}

	log_msg(LOG_INFO, "Server listening on port %d", config.port);

	while (1) {
		event_count = epoll_wait(epoll_fd, events, config.max_connections, -1);
		log_msg(LOG_DEBUG, "Epoll wait returned %d", event_count);
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
					int keep_alive = 1;
					char method[16], path[256], protocol[16];
					sscanf(buffer, "%s %s %s", method, path, protocol);
					printf("Received request: %s %s %s\n", method, path, protocol);
					if (strstr(buffer, "Connection: close")) {
						keep_alive = 0;
					}
					if (strncmp(path, "/storage", 8) == 0) {
						if (strcmp(method, "PUT") == 0) {
							handle_file_upload(client_fd, path, buffer, bytes_read);
						} else if (strcmp(method, "GET") == 0) {
							handle_file_download(client_fd, path);
						} else {
							send_error_html(client_fd, "file/405.html", 405);
						}
					} else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
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
					} else if (strcmp(path, "/post-test") == 0) {
						http_post("https://httpbin.org/post", client_fd);
					} else if (strcmp(path, "/delete-test") == 0) {
						http_delete("https://httpbin.org/delete", client_fd);
					} else if (strcmp(path, "/put-test") == 0) {
						http_put("https://httpbin.org/put", "test_file.txt", client_fd);
					} else {
						send_error_html(client_fd, "file/404.html", 404);
					}
					if (keep_alive == 0) {
						close(client_fd);
					}
				}
			}
		}
	}

	logger_close();
	close(server_socket);
	return client_socket;
}
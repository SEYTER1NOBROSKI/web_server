#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../include/memory.h"

#define BUFFER_SIZE 1024

void send_html(int client_socket, const char *file_path) {
	FILE *html_file = fopen(file_path, "r");
	if (html_file == NULL) {
		printf("Could not open file %s %d %s\n", file_path, errno, strerror(errno));
		return;
	}

	char buffer[BUFFER_SIZE];
	size_t bytes_read;
	char *http_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	send(client_socket, http_header, strlen(http_header), 0);

	while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, html_file)) > 0) {
		send(client_socket, buffer, bytes_read, 0);
	}
	fclose(html_file);
}

void send_error_html(int client_socket, const char *file_path, long http_code) {
	const char *http_header;

	switch (http_code) {
		case 400:
			http_header = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n";
			break;
		case 403:
			http_header = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n";
			break;
		case 404:
			http_header = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
			break;
		case 501:
			http_header = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html\r\n\r\n";
			break;
		case 503:
			http_header = "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/html\r\n\r\n";
			break;
		default:
			http_header = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\n\r\n";
			break;
	}

	send(client_socket, http_header, strlen(http_header), 0);

	FILE *html_file = fopen(file_path, "r");
	if (html_file == NULL) {
		printf("ERROR: Could not open error file %s\n", file_path);
		char *fallback_msg = "<h1>Error occurred, but error page is missing.</h1>";
		send(client_socket, fallback_msg, strlen(fallback_msg), 0);
		return;
	}

	char buffer[BUFFER_SIZE];
	size_t bytes_read;

	while ((bytes_read = fread(buffer, 1, sizeof(buffer), html_file)) > 0) {
		send(client_socket, buffer, bytes_read, 0);
	}

	fclose(html_file);
}

void handle_client_response(int client_socket, long http_code, struct MemoryStruct *data) {
	if (http_code == 404) {
		send_error_html(client_socket, "file/404.html", 404);
	} else if (http_code == 403) {
		send_error_html(client_socket, "file/403.html", 403);
	} else if (http_code == 400) {
		send_error_html(client_socket, "file/400.html", 400);
	} else if (http_code == 501) {
		send_error_html(client_socket, "file/501.html", 501);
	} else if (http_code == 503) {
		send_error_html(client_socket, "file/503.html", 503);
	}
}
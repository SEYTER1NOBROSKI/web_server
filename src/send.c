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
		case 201:
			http_header = "HTTP/1.1 201 Created\r\nContent-Type: text/html\r\n\r\n";
			break;
		case 204:
			http_header = "HTTP/1.1 204 No Content\r\nContent-Type: text/html\r\n\r\n";
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
	} else if (http_code == 201) {
		send_html(client_socket, "file/201.html");
	} else if (http_code == 204) {
		send_html(client_socket, "file/204.html");
	}
}

void handle_file_upload(int client_socket, char *path, char *buffer, size_t bytes_read) {
	char file_path[512];
	snprintf(file_path, sizeof(file_path), "%s", path + 1);
	printf("Saving uploaded file to %s\n", file_path);

	char *len_str = strstr(buffer, "Content-Length: ");
	long content_length = 0;
	if (len_str) {
		content_length = strtol(len_str + 16, NULL, 10);
	}
	
	if (content_length <= 0) {
		char *msg = "HTTP/1.1 411 Length Required\r\n\r\nLength Required";
		send(client_socket, msg, strlen(msg), 0);
		return;
	}

	char *body_start = strstr(buffer, "\r\n\r\n");
	if (!body_start) {
		printf("Error: Could not find end of headers\n");
		return;
	}
	body_start += 4;

	long header_len = body_start - buffer;
	long bytes_in_buffer = bytes_read - header_len;

	FILE *fp = fopen(file_path, "wb");
	if (!fp) {
		perror("File open error");
		char *msg = "HTTP/1.1 500 Internal Server Error\r\n\r\nCannot open file";
		send(client_socket, msg, strlen(msg), 0);
		return;
	}

	if (bytes_in_buffer > 0) {
		fwrite(body_start, 1, bytes_in_buffer, fp);
		content_length -= bytes_in_buffer;
	}

	char data_buffer[BUFFER_SIZE];
	ssize_t received;
	while (content_length > 0) {
		received = recv(client_socket, data_buffer, sizeof(data_buffer), 0);
		if (received <= 0) break;
		fwrite(data_buffer, 1, received, fp);
		content_length -= received;
	}

	fclose(fp);

	char *msg = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
	send(client_socket, msg, strlen(msg), 0);
	printf("File saved successfully.\n");
}
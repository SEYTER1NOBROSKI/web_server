#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
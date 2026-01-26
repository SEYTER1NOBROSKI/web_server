#include "../include/init_server.h"
#include "../include/http_methods.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

int client_socket;

void* server_thread_func() {
	client_socket = setup_server();
	return NULL;
}

int main() {
	pthread_t server_tid;
	if (pthread_create(&server_tid, NULL, server_thread_func, NULL) != 0) {
		printf("Failed to create server thread\n");
		return 1;
	}

	sleep(2);

	//http_get("http://localhost:8080/", client_socket);
	// Сервер відповість кодом 404
	//http_get("https://httpbin.org/status/404", client_socket);

	pthread_join(server_tid, NULL);
	return 0;
}
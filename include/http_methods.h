#include <stddef.h>

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

void http_get(const char* url, int client_socket);

void http_post(const char* url, int client_socket);

void http_delete(const char* url, int client_socket);

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

void http_put(const char* url, const char* file_path, int client_socket);
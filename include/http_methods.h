#include <stddef.h>

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);

void http_get(const char* url, int client_socket);

void http_post(const char* url);

void http_delete(const char* url);
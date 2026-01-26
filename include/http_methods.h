#include <stddef.h>

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

void http_get(const char* url, int client_socket);
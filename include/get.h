#include <stddef.h>

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

void http_get_example(const char* url);
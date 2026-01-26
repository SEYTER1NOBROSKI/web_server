#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "../include/memory.h"
#include "../include/send.h"

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		printf("not enough memory (realloc)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void http_get(const char* url, int client_socket) {
	CURL *curl_handle;
	CURLcode res; // result code
	struct MemoryStruct chunk;
	long http_code = 0;

	chunk.memory = malloc(1);
	chunk.memory[0] = '\0';
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			send(client_socket, chunk.memory, chunk.size, 0);
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
			handle_client_response(client_socket, http_code, &chunk);
			printf("HTTP Code: %ld\n", http_code);
			printf("Size: %lu bytes\n", (unsigned long)chunk.size);

			if (chunk.size > 0) {
				printf("Data: %s\n", chunk.memory);
			} else {
				printf("Data: [EMPTY BODY]\n");
			}
		}

		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
	}
	curl_global_cleanup();
}

void http_post(const char* url) {
	CURL *curl_handle;
	CURLcode res;
	char *post_data = "field1=value1&field2=value2";

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
}

void http_delete(const char* url) {
	CURL *curl_handle;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
}
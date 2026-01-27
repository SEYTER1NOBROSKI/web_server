#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
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

void http_post(const char* url, int client_socket) {
	CURL *curl_handle;
	CURLcode res;
	char *post_data = "field1=value1&field2=value2";
	int http_code = 0;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
			char success_msg[512];
			send(client_socket, success_msg, snprintf(success_msg, sizeof(success_msg), "HTTP POST request completed with status code %d", http_code), 0);
		}
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
}

void http_delete(const char* url, int client_socket) {
	CURL *curl_handle;
	CURLcode res;
	long http_code = 0;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
			char success_msg[512];
			send(client_socket, success_msg, snprintf(success_msg, sizeof(success_msg), "HTTP DELETE request completed with status code %ld", http_code), 0);
		}
		curl_easy_cleanup(curl_handle);
	}
	curl_global_cleanup();
}

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t retcode;
	unsigned long nread;

	retcode = fread(ptr, size, nmemb, userdata);
	if (retcode > 0) {
		nread = (unsigned long)(retcode * size);
		printf("Read %lu bytes from file\n", nread);
	} else {
		printf("End of file reached or read error. %s\n", strerror(errno));
	}

	return retcode;
}

void http_put(const char* url, const char* file_path, int client_socket) {
	CURL *curl_handle;
	CURLcode res;
	FILE *hd_src;
	struct stat file_info;
	int http_code = 0;

	if (stat(file_path, &file_info) == -1) {
		printf("Could not get file info: %s\n", strerror(errno));
		return;
	}

	hd_src = fopen(file_path, "rb");
	if (!hd_src) {
		printf("Could not open file %s: %s\n", file_path, strerror(errno));
		return;
	}

	res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		printf("curl_global_init() failed: %s\n", curl_easy_strerror(res));
		fclose(hd_src);
		return;
	}
	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_READDATA, hd_src);
		curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

		res = curl_easy_perform(curl_handle);
		if (res != CURLE_OK) {
			printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} else {
			printf("File %s uploaded successfully.\n", file_path);
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
			char success_msg[512];
			send(client_socket, success_msg, snprintf(success_msg, sizeof(success_msg), "HTTP PUT request completed with status code %d", http_code), 0);
		}

		curl_easy_cleanup(curl_handle);
	}
	fclose(hd_src);
	curl_global_cleanup();
}
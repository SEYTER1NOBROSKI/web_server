#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "../include/config.h"

char *read_file(const char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *content = malloc(length + 1);
	if (content) {
		fread(content, 1, length, file);
		content[length] = '\0';
	}
	fclose(file);
	return content;
}

int load_config(const char *filename, ServerConfig *config) {
	strcpy(config->ip, "0.0.0.0");
	config->port = 8080;
	config->max_connections = 5;
	strcpy(config->root_directory, "storage");

	char *json_string = read_file(filename);
	if (!json_string) {
		printf("[WARN] Config file not found, using defaults.\n");
		return -1;
	}

	cJSON *json = cJSON_Parse(json_string);
	if (!json) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			fprintf(stderr, "[ERROR] JSON Parse Error before: %s\n", error_ptr);
		}
		free(json_string);
		return -1;
	}

	cJSON *ip = cJSON_GetObjectItemCaseSensitive(json, "server_ip");
	if (cJSON_IsString(ip) && (ip->valuestring != NULL)) {
		strncpy(config->ip, ip->valuestring, sizeof(config->ip) - 1);
	}

	cJSON *port = cJSON_GetObjectItemCaseSensitive(json, "port");
	if (cJSON_IsNumber(port)) {
		config->port = port->valueint;
	}

	cJSON *max_con = cJSON_GetObjectItemCaseSensitive(json, "max_connections");
	if (cJSON_IsNumber(max_con)) {
		config->max_connections = max_con->valueint;
	}

	cJSON *root = cJSON_GetObjectItemCaseSensitive(json, "root_directory");
	if (cJSON_IsString(root) && (root->valuestring != NULL)) {
		strncpy(config->root_directory, root->valuestring, sizeof(config->root_directory) - 1);
	}

	cJSON_Delete(json);
	free(json_string);
	return 0;
}
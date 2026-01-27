#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	char ip[16];
	int port;
	int max_connections;
	char root_directory[256];
} ServerConfig;

int load_config(const char *filename, ServerConfig *config);

#endif
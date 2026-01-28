#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "../include/logger.h"

static LogLevel current_level = LOG_INFO;
static FILE *log_file_ptr = NULL;

static const char *level_strings[] = {
	"DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
	"\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m" // Cyan, Green, Yellow, Red, Magenta
};

void logger_init(LogLevel level, const char *file_path) {
	current_level = level;

	if (log_file_ptr) {
		fclose(log_file_ptr);
		log_file_ptr = NULL;
	}

	if (file_path && strlen(file_path) > 0) {
		log_file_ptr = fopen(file_path, "a");
		if (!log_file_ptr) {
			perror("Failed to open log file");
		}
	}
}

void logger_close() {
	if (log_file_ptr) {
		fclose(log_file_ptr);
		log_file_ptr = NULL;
	}
}

void log_msg(LogLevel level, const char *format, ...) {
	if (level < current_level) return;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char time_str[20];
	strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

	va_list args;

	printf("[%s] %s%-5s\x1b[0m: ", time_str, level_colors[level], level_strings[level]);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");

	if (log_file_ptr) {
		fprintf(log_file_ptr, "[%s] %-5s: ", time_str, level_strings[level]);

		va_start(args, format);
		vfprintf(log_file_ptr, format, args);
		va_end(args);

		fprintf(log_file_ptr, "\n");
		fflush(log_file_ptr);
	}
}
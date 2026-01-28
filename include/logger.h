#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL
} LogLevel;

void logger_init(LogLevel level, const char *file_path);
void logger_close();
void log_msg(LogLevel level, const char *format, ...);

#endif
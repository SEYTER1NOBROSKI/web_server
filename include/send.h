void send_html(int client_socket, const char *file_path);

void send_error_html(int client_socket, const char *file_path, long http_code);

void handle_client_response(int client_socket, long http_code, struct MemoryStruct *data);